#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

namespace standbyUsb {
namespace fs = std::filesystem;
struct Paths {
    fs::path sys = "/sys";
    fs::path model = "/proc/device-tree/model";
    fs::path mounts = "/proc/self/mountinfo";
    fs::path swaps = "/proc/swaps";
    fs::path state = "/run/apertar-standby-usb";
};
std::string read(const fs::path &path)
{
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read " + path.string());
    return {std::istreambuf_iterator<char>(file), {}};
}
void write(const fs::path &path, const std::string &value)
{
    std::ofstream file(path);
    file << value << '\n';
    file.flush();
    if (!file) throw std::runtime_error("Cannot write " + path.string());
}
bool usbPath(const fs::path &path)
{
    std::error_code error;
    const auto resolved = fs::canonical(path, error).string();
    return !error && resolved.find("/usb") != std::string::npos;
}
bool usbBlock(const Paths &paths, const fs::path &device, std::set<std::string> &seen)
{
    std::error_code error;
    const fs::path resolved = fs::canonical(device, error);
    if (error || !seen.insert(resolved.string()).second) return false;
    if (usbPath(resolved)) return true;
    const fs::path slaves = resolved / "slaves";
    if (fs::is_directory(slaves)) {
        for (const auto &slave : fs::directory_iterator(slaves))
            if (usbBlock(paths, slave.path(), seen)) return true;
    }
    return false;
}
std::vector<fs::path> ports(const Paths &paths)
{
    if (read(paths.model).find("Raspberry Pi 5 Model") != 0)
        throw std::runtime_error("Automatic USB power-off is supported only on Raspberry Pi 5");
    std::vector<fs::path> result;
    unsigned hubs = 0;
    for (const auto &entry : fs::directory_iterator(paths.sys / "bus/usb/devices")) {
        const std::string name = entry.path().filename();
        if (name.size() <= 3 || name.substr(0, 3) != "usb" ||
            name.find_first_not_of("0123456789", 3) != std::string::npos) continue;
        const auto real = fs::canonical(entry.path());
        const std::string full = real.string();
        if (full.find("/1f00200000.usb/") == std::string::npos &&
            full.find("/1f00300000.usb/") == std::string::npos) continue;
        ++hubs;
        const auto hub = real / (name.substr(3) + "-0:1.0");
        for (const auto &port : fs::directory_iterator(hub)) {
            const std::string label = port.path().filename();
            if (label.find(name + "-port") == 0 && fs::exists(port.path() / "disable"))
                result.push_back(port.path() / "disable");
        }
    }
    if (hubs != 4 || result.size() != 6)
        throw std::runtime_error("Unexpected Pi 5 USB topology; keeping USB powered");
    std::sort(result.begin(), result.end());
    return result;
}
void guard(const Paths &paths)
{
    // Major/minor lookup also catches storage mounted by UUID, by-id or /dev/mapper.
    std::istringstream mounts(read(paths.mounts));
    std::string line;
    while (std::getline(mounts, line)) {
        std::istringstream fields(line);
        std::string id, parent, device;
        fields >> id >> parent >> device;
        if (device.empty()) throw std::runtime_error("Invalid mount information");
        std::set<std::string> seen;
        if (usbBlock(paths, paths.sys / "dev/block" / device, seen))
            throw std::runtime_error("USB storage is mounted; USB power-off skipped");
        if (device.rfind("0:", 0) != 0 && !fs::exists(paths.sys / "dev/block" / device))
            throw std::runtime_error("Cannot verify a mounted block device; USB power-off skipped");
        const auto separator = line.find(" - ");
        if (separator == std::string::npos)
            throw std::runtime_error("Invalid mount information");
        std::istringstream filesystem(line.substr(separator + 3));
        std::string type, source;
        filesystem >> type >> source;
        // FUSE/NTFS can have a virtual major/minor. Check the backing block
        // source as well, including aliases and encrypted/device-mapper disks.
        if (source.rfind("/dev/", 0) == 0) {
            std::string decoded;
            for (size_t i = 0; i < source.size(); ++i) {
                if (source[i] == '\\' && i + 3 < source.size() &&
                    source[i+1] >= '0' && source[i+1] <= '7' &&
                    source[i+2] >= '0' && source[i+2] <= '7' &&
                    source[i+3] >= '0' && source[i+3] <= '7') {
                    decoded += char((source[i+1]-'0')*64 + (source[i+2]-'0')*8 + source[i+3]-'0');
                    i += 3;
                } else decoded += source[i];
            }
            struct stat info {};
            if (::stat(decoded.c_str(), &info) == 0 && S_ISBLK(info.st_mode)) {
                seen.clear();
                const auto backing = paths.sys / "dev/block" /
                    (std::to_string(major(info.st_rdev)) + ":" + std::to_string(minor(info.st_rdev)));
                if (usbBlock(paths, backing, seen))
                    throw std::runtime_error("USB-backed filesystem is mounted; USB power-off skipped");
            } else if (device.rfind("0:", 0) == 0)
                throw std::runtime_error("Cannot verify a mounted filesystem source; USB power-off skipped");
        }
        // Btrfs can expose a virtual major/minor instead of its underlying disks.
        if (type == "btrfs" || type == "zfs" || type == "fuseblk") {
            for (const auto &block : fs::directory_iterator(paths.sys / "class/block"))
                if (usbPath(block.path()))
                    throw std::runtime_error("Multi-device/FUSE filesystem and USB storage present; USB power-off skipped");
        }
    }
    std::istringstream swaps(read(paths.swaps));
    std::getline(swaps, line);
    while (std::getline(swaps, line)) {
        std::istringstream fields(line);
        std::string device;
        fields >> device;
        struct stat info {};
        if (::stat(device.c_str(), &info) == 0) {
            // Swap files live on an already checked mounted filesystem.
            if (S_ISBLK(info.st_mode)) {
                const auto block = paths.sys / "dev/block" /
                    (std::to_string(major(info.st_rdev)) + ":" + std::to_string(minor(info.st_rdev)));
                std::set<std::string> seen;
                if (usbBlock(paths, block, seen))
                    throw std::runtime_error("USB swap is active; USB power-off skipped");
            }
        } else if (!device.empty())
            throw std::runtime_error("Cannot verify active swap; USB power-off skipped");
    }
    for (const auto &network : fs::directory_iterator(paths.sys / "class/net")) {
        if (usbPath(network.path() / "device"))
            throw std::runtime_error("USB network adapter present; USB power-off skipped");
    }
}
using Snapshot = std::vector<std::pair<fs::path, std::string>>;
Snapshot snapshot(const std::vector<fs::path> &targets)
{
    Snapshot result;
    for (const auto &target : targets) {
        const std::string value = read(target).substr(0, 1);
        if (value != "0" && value != "1") throw std::runtime_error("Invalid USB port state");
        result.emplace_back(target, value);
    }
    return result;
}
void restore(const Snapshot &previous)
{
    bool failed = false;
    for (const auto &[path, value] : previous) {
        try { write(path, value); } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            failed = true;
        }
    }
    if (failed) throw std::runtime_error("USB restore incomplete; saved state retained for retry");
}
void save(const Paths &paths, const Snapshot &previous)
{
    std::ofstream file(paths.state / "ports.tmp");
    for (const auto &[path, value] : previous)
        file << path.parent_path().filename().string() << ' ' << value << '\n';
    file.flush();
    if (!file) throw std::runtime_error("Could not save USB recovery state");
    file.close();
    fs::rename(paths.state / "ports.tmp", paths.state / "ports");
}
Snapshot load(const Paths &paths, const std::vector<fs::path> &targets)
{
    Snapshot previous;
    std::istringstream file(read(paths.state / "ports"));
    std::string label, value;
    std::set<std::string> seen;
    while (file >> label >> value) {
        const auto target = std::find_if(targets.begin(), targets.end(), [&](const fs::path &path) {
            return path.parent_path().filename() == label;
        });
        if (target == targets.end() || (value != "0" && value != "1") || !seen.insert(label).second)
            throw std::runtime_error("Invalid USB recovery state; refusing arbitrary sysfs writes");
        previous.emplace_back(*target, value);
    }
    if (previous.size() != targets.size()) throw std::runtime_error("Incomplete USB recovery state");
    return previous;
}
int operate(const Paths &paths, const std::string &action)
{
    // Wake/recovery is harmless on machines without an outstanding off request.
    if (action == "on" && !fs::exists(paths.state / "ports")) return 0;
    const auto targets = ports(paths);
    if (action == "on") {
        restore(load(paths, targets));
        fs::remove(paths.state / "ports");
        std::cout << "USB port power restored\n";
        return 0;
    }
    guard(paths);
    if (action == "check") {
        std::cout << "Pi 5 USB power-off safety checks passed\n";
        return 0;
    }
    if (fs::exists(paths.state / "ports")) return 0;
    const auto previous = snapshot(targets);
    save(paths, previous);
    ::sync();
    try {
        guard(paths);
        for (const auto &target : targets) write(target, "1");
    } catch (...) {
        restore(previous);
        fs::remove(paths.state / "ports");
        throw;
    }
    std::cout << "All Pi 5 onboard USB ports disabled; recovery state saved\n";
    return 0;
}
}

#ifndef APERTAR_USB_POWER_TEST
int main(int argc, char **argv)
{
    if (argc != 2 || (std::string(argv[1]) != "on" && std::string(argv[1]) != "off" &&
                      std::string(argv[1]) != "check")) {
        std::cerr << "Usage: apertar-standby-usb check|off|on\n";
        return 1;
    }
    const std::string action = argv[1];
    const standbyUsb::Paths paths;
    int lock = -1;
    try {
        if (action != "check") {
            if (::geteuid() != 0) throw std::runtime_error("USB power control requires the installed privileged helper");
            if (::mkdir(paths.state.c_str(), 0755) != 0 && errno != EEXIST)
                throw std::runtime_error("Cannot create USB recovery directory");
            struct stat info {};
            if (::lstat(paths.state.c_str(), &info) != 0 || !S_ISDIR(info.st_mode) ||
                info.st_uid != 0 || (info.st_mode & 0022))
                throw std::runtime_error("Unsafe USB recovery directory");
            lock = ::open((paths.state / "lock").c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
            if (lock < 0 || ::flock(lock, LOCK_EX | LOCK_NB) != 0)
                throw std::runtime_error("Another USB power operation is active");
        }
        const int result = standbyUsb::operate(paths, action);
        if (lock >= 0) ::close(lock);
        return result;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        if (lock >= 0) ::close(lock);
        return 1;
    }
}
#endif
