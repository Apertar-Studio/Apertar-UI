#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/mount.h>
#include <linux/fs.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace
{
constexpr int kDefaultI2cBus = 10;
constexpr int kDefaultI2cAddress = 0x34;
constexpr char kReadyPath[] = "/run/apertar-cfe/ready";

std::atomic_bool g_running{true};

void handleSignal(int)
{
    g_running.store(false, std::memory_order_relaxed);
}

std::string trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string configValue(std::string value)
{
    const auto hash = value.find('#');
    const auto semicolon = value.find(';');
    const auto comment = std::min(hash == std::string::npos ? value.size() : hash,
                                  semicolon == std::string::npos ? value.size() : semicolon);
    value = trim(value.substr(0, comment));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        value = value.substr(1, value.size() - 2);
    return value;
}

bool parseBool(const std::string &value, bool fallback)
{
    const std::string normalized = lower(configValue(value));
    if (normalized == "1" || normalized == "true" || normalized == "yes" ||
        normalized == "on" || normalized == "enabled") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" ||
        normalized == "off" || normalized == "disabled") {
        return false;
    }
    return fallback;
}

int parseInteger(const std::string &value, int fallback)
{
    try {
        std::size_t consumed = 0;
        const std::string normalized = configValue(value);
        const long parsed = std::stol(normalized, &consumed, 0);
        return consumed == normalized.size() ? static_cast<int>(parsed) : fallback;
    } catch (...) {
        return fallback;
    }
}

struct Config
{
    bool enabled = true;
    int i2cBus = kDefaultI2cBus;
    int i2cAddress = kDefaultI2cAddress;
    std::string mountPath = "/media/RAW";
    std::string pcieController = "1000110000.pcie";
    std::string sourcePath;
};

std::vector<std::string> configCandidates()
{
    std::vector<std::string> candidates;
    if (const char *configured = std::getenv("APERTAR_HARDWARE_CONFIG"); configured && *configured)
        candidates.emplace_back(configured);
    candidates.emplace_back("/home/pi/apertar-hardware.conf");
    candidates.emplace_back("/boot/firmware/apertar-hardware.conf");
    candidates.emplace_back("/boot/apertar-hardware.conf");

    std::vector<std::string> unique;
    for (const std::string &candidate : candidates) {
        if (std::find(unique.begin(), unique.end(), candidate) == unique.end())
            unique.push_back(candidate);
    }
    return unique;
}

Config loadConfig()
{
    Config config;
    std::ifstream input;
    for (const std::string &candidate : configCandidates()) {
        input.open(candidate);
        if (input) {
            config.sourcePath = candidate;
            break;
        }
        input.clear();
    }

    if (!input)
        return config;

    std::string section;
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = lower(trim(line.substr(1, line.size() - 2)));
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos)
            continue;
        const std::string key = lower(trim(line.substr(0, separator)));
        const std::string value = line.substr(separator + 1);

        if (section == "cfexpress_hat") {
            if (key == "enabled")
                config.enabled = parseBool(value, config.enabled);
            else if (key == "bus" || key == "channel" || key == "i2c_bus")
                config.i2cBus = parseInteger(value, config.i2cBus);
            else if (key == "address" || key == "addr")
                config.i2cAddress = parseInteger(value, config.i2cAddress);
        } else if (section == "storage") {
            if (key == "mount_path") {
                const std::string parsed = configValue(value);
                if (!parsed.empty())
                    config.mountPath = parsed;
            } else if (key == "pcie_controller") {
                const std::string parsed = configValue(value);
                if (!parsed.empty())
                    config.pcieController = parsed;
            }
        }
    }

    if (config.i2cBus < 0 || config.i2cBus > 255)
        config.i2cBus = kDefaultI2cBus;
    if (config.i2cAddress < 0x03 || config.i2cAddress > 0x77)
        config.i2cAddress = kDefaultI2cAddress;
    if (config.mountPath.empty() || config.mountPath.front() != '/')
        config.mountPath = "/media/RAW";
    return config;
}

struct CommandResult
{
    int exitCode = -1;
    std::string output;
};

CommandResult runCommand(const std::vector<std::string> &arguments)
{
    CommandResult result;
    if (arguments.empty())
        return result;

    int outputPipe[2] = {-1, -1};
    if (::pipe2(outputPipe, O_CLOEXEC) != 0)
        return result;

    const pid_t child = ::fork();
    if (child == 0) {
        ::close(outputPipe[0]);
        ::dup2(outputPipe[1], STDOUT_FILENO);
        ::dup2(outputPipe[1], STDERR_FILENO);
        ::close(outputPipe[1]);

        std::vector<char *> argv;
        argv.reserve(arguments.size() + 1);
        for (const std::string &argument : arguments)
            argv.push_back(const_cast<char *>(argument.c_str()));
        argv.push_back(nullptr);
        ::execv(argv.front(), argv.data());
        _exit(127);
    }

    ::close(outputPipe[1]);
    if (child < 0) {
        ::close(outputPipe[0]);
        return result;
    }

    char buffer[1024];
    ssize_t count = 0;
    while ((count = ::read(outputPipe[0], buffer, sizeof(buffer))) > 0)
        result.output.append(buffer, static_cast<std::size_t>(count));
    ::close(outputPipe[0]);

    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
    if (WIFEXITED(status))
        result.exitCode = WEXITSTATUS(status);
    return result;
}

bool writeTextFile(const fs::path &path, const std::string &value)
{
    std::ofstream output(path);
    if (!output) {
        std::cerr << "CFE automounter: cannot open " << path << ": " << std::strerror(errno) << std::endl;
        return false;
    }
    output << value;
    output.flush();
    if (!output) {
        std::cerr << "CFE automounter: cannot write " << path << std::endl;
        return false;
    }
    return true;
}

std::string decodeMountField(const std::string &field)
{
    std::string decoded;
    decoded.reserve(field.size());
    for (std::size_t index = 0; index < field.size(); ++index) {
        if (field[index] == '\\' && index + 3 < field.size() &&
            std::isdigit(static_cast<unsigned char>(field[index + 1])) &&
            std::isdigit(static_cast<unsigned char>(field[index + 2])) &&
            std::isdigit(static_cast<unsigned char>(field[index + 3]))) {
            const int value = (field[index + 1] - '0') * 64 +
                              (field[index + 2] - '0') * 8 +
                              (field[index + 3] - '0');
            decoded.push_back(static_cast<char>(value));
            index += 3;
        } else {
            decoded.push_back(field[index]);
        }
    }
    return decoded;
}

std::string mountedSourceForPath(const std::string &mountPath)
{
    std::ifstream input("/proc/self/mountinfo");
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream stream(line);
        std::vector<std::string> fields;
        std::string field;
        while (stream >> field)
            fields.push_back(field);
        if (fields.size() < 10 || decodeMountField(fields[4]) != mountPath)
            continue;

        const auto separator = std::find(fields.begin() + 6, fields.end(), "-");
        if (separator != fields.end() && std::distance(separator, fields.end()) >= 3)
            return decodeMountField(*(separator + 2));
    }
    return {};
}

struct PartitionKey
{
    int controller = 0;
    int nameSpace = 0;
    int partition = 0;
    std::string device;
};

std::string findNvmePartition()
{
    static const std::regex pattern(R"(^nvme([0-9]+)n([0-9]+)p([0-9]+)$)");
    std::vector<PartitionKey> partitions;
    std::error_code error;
    for (const fs::directory_entry &entry : fs::directory_iterator("/sys/class/block", error)) {
        const std::string name = entry.path().filename().string();
        std::smatch match;
        if (!std::regex_match(name, match, pattern) || !fs::exists(entry.path() / "partition"))
            continue;
        partitions.push_back({std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]), "/dev/" + name});
    }
    if (partitions.empty())
        return {};
    std::sort(partitions.begin(), partitions.end(), [](const PartitionKey &left, const PartitionKey &right) {
        return std::tie(left.controller, left.nameSpace, left.partition) <
               std::tie(right.controller, right.nameSpace, right.partition);
    });
    return partitions.back().device;
}

std::string waitForNvmePartition(std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    do {
        const std::string device = findNvmePartition();
        if (!device.empty())
            return device;
        std::this_thread::sleep_for(150ms);
    } while (g_running.load(std::memory_order_relaxed) && std::chrono::steady_clock::now() < deadline);
    return {};
}

std::string pciAddressForBlockDevice(const std::string &device)
{
    static const std::regex pattern(R"(^[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-7]$)");
    const fs::path sysPath = fs::path("/sys/class/block") / fs::path(device).filename();
    std::error_code error;
    const fs::path resolved = fs::canonical(sysPath, error);
    if (error)
        return {};
    std::string result;
    for (const fs::path &component : resolved) {
        const std::string value = component.string();
        if (std::regex_match(value, pattern))
            result = value;
    }
    return result;
}

std::string findNvmePciAddress()
{
    std::vector<std::string> devices;
    std::error_code error;
    for (const fs::directory_entry &entry : fs::directory_iterator("/sys/bus/pci/devices", error)) {
        std::ifstream classFile(entry.path() / "class");
        std::string classCode;
        classFile >> classCode;
        if (classCode.rfind("0x0108", 0) == 0)
            devices.push_back(entry.path().filename().string());
    }
    if (devices.empty())
        return {};
    std::sort(devices.begin(), devices.end());
    return devices.back();
}

std::string filesystemType(const std::string &device)
{
    for (const char *program : {"/usr/sbin/blkid", "/sbin/blkid", "/usr/bin/blkid"}) {
        if (::access(program, X_OK) != 0)
            continue;
        CommandResult result = runCommand({program, "-s", "TYPE", "-o", "value", device});
        if (result.exitCode == 0)
            return lower(trim(result.output));
    }
    return {};
}

class HatInterface
{
public:
    HatInterface(int bus, int address)
        : m_devicePath("/dev/i2c-" + std::to_string(bus)), m_address(address)
    {
    }

    ~HatInterface()
    {
        closeDevice();
    }

    std::optional<std::pair<bool, bool>> readButtons()
    {
        if (!ensureOpen())
            return std::nullopt;

        union i2c_smbus_data data {};
        struct i2c_smbus_ioctl_data request {
            I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data
        };
        if (::ioctl(m_fd, I2C_SMBUS, &request) < 0) {
            handleIoError("read");
            return std::nullopt;
        }
        markIoSuccess();
        if (data.byte == 0x69)
            return std::nullopt;

        return std::make_pair((data.byte & 0x01u) != 0u,
                              (data.byte & 0x02u) != 0u);
    }

    bool writeLed(bool enabled)
    {
        for (int attempt = 0; attempt < 5; ++attempt) {
            if (ensureOpen()) {
                struct i2c_smbus_ioctl_data request {
                    I2C_SMBUS_WRITE,
                    static_cast<std::uint8_t>(enabled ? 1 : 0),
                    I2C_SMBUS_BYTE,
                    nullptr
                };
                if (::ioctl(m_fd, I2C_SMBUS, &request) >= 0) {
                    markIoSuccess();
                    return true;
                }
                handleIoError("write");
            }
            std::this_thread::sleep_for(100ms);
        }
        return false;
    }

private:
    bool ensureOpen()
    {
        if (m_fd >= 0)
            return true;
        const auto now = std::chrono::steady_clock::now();
        if (now < m_nextOpenAttempt)
            return false;
        m_nextOpenAttempt = now + 1s;

        m_fd = ::open(m_devicePath.c_str(), O_RDWR | O_CLOEXEC);
        if (m_fd < 0) {
            logUnavailable();
            return false;
        }
        if (::ioctl(m_fd, I2C_SLAVE, m_address) < 0) {
            handleIoError("select address");
            return false;
        }
        return true;
    }

    void markIoSuccess()
    {
        if (m_unavailableLogged) {
            std::cout << "CFE automounter: I2C connection restored on " << m_devicePath << std::endl;
            m_unavailableLogged = false;
        }
    }

    void logUnavailable()
    {
        if (!m_unavailableLogged) {
            std::cerr << "CFE automounter: waiting for " << m_devicePath
                      << " address=0x" << std::hex << m_address << std::dec
                      << ": " << std::strerror(errno) << std::endl;
            m_unavailableLogged = true;
        }
    }

    void handleIoError(const char *operation)
    {
        if (!m_unavailableLogged) {
            std::cerr << "CFE automounter: I2C " << operation << " failed: "
                      << std::strerror(errno) << std::endl;
            m_unavailableLogged = true;
        }
        closeDevice();
        // A missing or disabled HAT should not continuously hammer a shared
        // camera I2C bus. Resume the normal 100 ms polling as soon as a slow
        // recovery probe succeeds.
        m_nextOpenAttempt = std::chrono::steady_clock::now() + 5s;
    }

    void closeDevice()
    {
        if (m_fd >= 0)
            ::close(m_fd);
        m_fd = -1;
    }

    std::string m_devicePath;
    int m_address = kDefaultI2cAddress;
    int m_fd = -1;
    bool m_unavailableLogged = false;
    std::chrono::steady_clock::time_point m_nextOpenAttempt {};
};

class Automounter
{
public:
    explicit Automounter(Config config)
        : m_config(std::move(config)), m_hat(m_config.i2cBus, m_config.i2cAddress)
    {
    }

    int run()
    {
        refreshMountState(false);
        ensureStartupMount();
        m_hat.writeLed(m_mounted);

        auto initialButtons = waitForButtons(500ms);
        bool lastInsert = initialButtons ? initialButtons->first : true;
        bool lastEject = initialButtons ? initialButtons->second : true;

        fs::create_directories(fs::path(kReadyPath).parent_path());
        writeTextFile(kReadyPath, "ready\n");
        std::cout << "CFE automounter: ready mounted=" << (m_mounted ? "true" : "false")
                  << " source=" << (m_mountedDevice.empty() ? "none" : m_mountedDevice)
                  << std::endl;

        auto nextMountRefresh = std::chrono::steady_clock::now();
        while (g_running.load(std::memory_order_relaxed)) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= nextMountRefresh) {
                refreshMountState(true);
                nextMountRefresh = now + 500ms;
            }

            if (const auto buttons = m_hat.readButtons()) {
                const bool insert = buttons->first;
                const bool eject = buttons->second;
                if (lastInsert && !insert && !m_mounted)
                    mountPcie();
                if (lastEject && !eject)
                    unmountPcie();
                lastInsert = insert;
                lastEject = eject;
            }
            std::this_thread::sleep_for(100ms);
        }

        m_hat.writeLed(false);
        std::error_code error;
        fs::remove(kReadyPath, error);
        return 0;
    }

private:
    std::optional<std::pair<bool, bool>> waitForButtons(std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        do {
            if (const auto buttons = m_hat.readButtons())
                return buttons;
            std::this_thread::sleep_for(100ms);
        } while (g_running.load(std::memory_order_relaxed) && std::chrono::steady_clock::now() < deadline);
        return std::nullopt;
    }

    void refreshMountState(bool updateLed)
    {
        const std::string source = mountedSourceForPath(m_config.mountPath);
        const bool mounted = !source.empty();
        if (mounted)
            m_lastMountedDevice = source;
        if (mounted != m_mounted || source != m_mountedDevice) {
            m_mounted = mounted;
            m_mountedDevice = source;
            std::cout << "CFE automounter: mount state=" << (m_mounted ? "mounted" : "unmounted")
                      << " source=" << (source.empty() ? "none" : source) << std::endl;
            if (updateLed)
                m_hat.writeLed(m_mounted);
        }
    }

    bool mountPartition(const std::string &device)
    {
        if (!mountedSourceForPath(m_config.mountPath).empty())
            return true;

        const std::string detectedType = filesystemType(device);
        std::string mountType = detectedType;
        std::string options;
        if (detectedType == "ntfs") {
            mountType = "ntfs3";
            options = "uid=1000,gid=1000";
        } else if (detectedType == "exfat" || detectedType == "vfat") {
            options = "uid=1000,gid=1000,umask=0002";
        } else if (detectedType != "ext4") {
            std::cerr << "CFE automounter: unsupported filesystem on " << device
                      << ": " << (detectedType.empty() ? "unknown" : detectedType) << std::endl;
            return false;
        }

        std::error_code error;
        fs::create_directories(m_config.mountPath, error);
        if (error) {
            std::cerr << "CFE automounter: cannot create " << m_config.mountPath
                      << ": " << error.message() << std::endl;
            return false;
        }

        const void *data = options.empty() ? nullptr : options.c_str();
        if (::mount(device.c_str(), m_config.mountPath.c_str(), mountType.c_str(), MS_NOATIME, data) != 0) {
            std::cerr << "CFE automounter: mount failed for " << device << " at "
                      << m_config.mountPath << ": " << std::strerror(errno) << std::endl;
            return false;
        }

        std::cout << "CFE automounter: mounted " << device << " at " << m_config.mountPath
                  << " type=" << mountType << std::endl;
        m_lastMountedDevice = device;
        refreshMountState(false);
        return true;
    }

    bool resetPcieController()
    {
        std::cout << "CFE automounter: resetting PCIe controller " << m_config.pcieController << std::endl;
        const fs::path driverRoot("/sys/bus/platform/drivers/brcm-pcie");
        if (fs::exists(driverRoot / m_config.pcieController)) {
            if (!writeTextFile(driverRoot / "unbind", m_config.pcieController + "\n"))
                return false;
            std::this_thread::sleep_for(100ms);
        }
        if (!writeTextFile(driverRoot / "bind", m_config.pcieController + "\n"))
            return false;
        if (!waitForNvmePartition(2s).empty())
            return true;
        if (!writeTextFile("/sys/bus/pci/rescan", "1\n"))
            return false;
        return !waitForNvmePartition(3s).empty();
    }

    bool mountPcie()
    {
        std::cout << "CFE automounter: insert requested" << std::endl;
        m_hat.writeLed(false);
        refreshMountState(false);
        if (m_mounted)
            return true;

        std::string partition = findNvmePartition();
        if (!partition.empty() && mountPartition(partition)) {
            m_hat.writeLed(true);
            return true;
        }

        if (!resetPcieController()) {
            std::cerr << "CFE automounter: PCIe reset did not produce an NVMe partition" << std::endl;
            return false;
        }
        partition = waitForNvmePartition(1s);
        const bool mounted = !partition.empty() && mountPartition(partition);
        m_hat.writeLed(mounted);
        return mounted;
    }

    void flushMedia(const std::string &device)
    {
        const int mountFd = ::open(m_config.mountPath.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (mountFd >= 0) {
            if (::syncfs(mountFd) != 0)
                std::cerr << "CFE automounter: syncfs failed: " << std::strerror(errno) << std::endl;
            ::close(mountFd);
        } else {
            ::sync();
        }

        if (!device.empty()) {
            const int deviceFd = ::open(device.c_str(), O_RDONLY | O_CLOEXEC);
            if (deviceFd >= 0) {
                ::ioctl(deviceFd, BLKFLSBUF, 0);
                ::close(deviceFd);
            }
        }
    }

    bool unmountPcie()
    {
        refreshMountState(false);
        const std::string device = m_mountedDevice.empty() ? m_lastMountedDevice : m_mountedDevice;
        std::cout << "CFE automounter: eject requested source="
                  << (device.empty() ? "none" : device) << std::endl;

        if (m_mounted) {
            flushMedia(device);
            std::this_thread::sleep_for(500ms);
            if (::umount2(m_config.mountPath.c_str(), 0) != 0) {
                std::cerr << "CFE automounter: unmount failed for " << m_config.mountPath
                          << ": " << std::strerror(errno) << std::endl;
                refreshMountState(true);
                return false;
            }
        }

        std::string pciAddress = pciAddressForBlockDevice(device);
        if (pciAddress.empty())
            pciAddress = findNvmePciAddress();
        if (!pciAddress.empty()) {
            const fs::path removePath = fs::path("/sys/bus/pci/devices") / pciAddress / "remove";
            if (!writeTextFile(removePath, "1\n")) {
                refreshMountState(true);
                return false;
            }
            if (::access("/usr/bin/udevadm", X_OK) == 0)
                runCommand({"/usr/bin/udevadm", "settle"});
        }

        m_mounted = false;
        m_mountedDevice.clear();
        m_hat.writeLed(false);
        std::cout << "CFE automounter: media safely ejected" << std::endl;
        return true;
    }

    void ensureStartupMount()
    {
        if (m_mounted)
            return;
        if (!findNvmePartition().empty()) {
            mountPcie();
            return;
        }
        if (!findNvmePciAddress().empty()) {
            mountPcie();
            return;
        }
        std::cout << "CFE automounter: no startup media detected; probing PCIe once" << std::endl;
        mountPcie();
    }

    Config m_config;
    HatInterface m_hat;
    bool m_mounted = false;
    std::string m_mountedDevice;
    std::string m_lastMountedDevice;
};
}

int main(int argc, char **argv)
{
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const Config config = loadConfig();
    std::cout << "CFE automounter: config="
              << (config.sourcePath.empty() ? "defaults" : config.sourcePath)
              << " enabled=" << (config.enabled ? "true" : "false")
              << " bus=" << config.i2cBus
              << " address=0x" << std::hex << config.i2cAddress << std::dec
              << " mount=" << config.mountPath
              << " controller=" << config.pcieController << std::endl;

    if (argc > 1 && std::string(argv[1]) == "--check-config")
        return 0;
    if (!config.enabled) {
        std::cout << "CFE automounter: disabled by configuration" << std::endl;
        return 0;
    }

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    Automounter automounter(config);
    return automounter.run();
}
