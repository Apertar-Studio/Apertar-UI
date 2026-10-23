#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace lowPower {
namespace fs = std::filesystem;
struct Paths {
    fs::path sys = "/sys";
    fs::path state = "/run/apertar-standby-low-power";
};
struct Setting { fs::path path; std::string target; };
using Settings = std::map<std::string, Setting>;
std::string read(const fs::path &path)
{
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read " + path.string());
    std::string value;
    std::getline(file, value);
    return value;
}
void write(const fs::path &path, const std::string &value)
{
    std::ofstream file(path);
    file << value << '\n';
    file.flush();
    if (!file) throw std::runtime_error("Cannot write " + path.string());
}
bool numbered(const std::string &name, const std::string &prefix)
{
    return name.size() > prefix.size() && name.rfind(prefix, 0) == 0 &&
        name.find_first_not_of("0123456789", prefix.size()) == std::string::npos;
}
Settings candidates(const Paths &paths, const std::string &mode)
{
    Settings settings;
    const auto cpu = paths.sys / "devices/system/cpu/cpufreq";
    if ((mode == "all" || mode == "cpu") && fs::is_directory(cpu)) {
        for (const auto &entry : fs::directory_iterator(cpu)) {
            const auto name = entry.path().filename().string();
            if (numbered(name, "policy") &&
                read(entry.path() / "scaling_available_governors").find("powersave") != std::string::npos)
                settings.emplace("cpu/" + name, Setting{entry.path() / "scaling_governor", "powersave"});
        }
    }
    const auto nvme = paths.sys / "class/nvme";
    if ((mode == "all" || mode == "nvme") && fs::is_directory(nvme)) {
        for (const auto &entry : fs::directory_iterator(nvme)) {
            const auto name = entry.path().filename().string();
            if (!numbered(name, "nvme")) continue;
            std::error_code error;
            const auto device = fs::canonical(entry.path() / "device", error);
            if (error || !fs::exists(device / "class") ||
                read(device / "class") != "0x010802" || !fs::exists(device / "power/control"))
                continue;
            settings.emplace("nvme/" + name, Setting{device / "power/control", "auto"});
        }
    }
    const auto radio = paths.sys / "class/rfkill";
    if ((mode == "all" || mode == "bluetooth") && fs::is_directory(radio)) {
        for (const auto &entry : fs::directory_iterator(radio)) {
            const auto name = entry.path().filename().string();
            if (numbered(name, "rfkill") && read(entry.path() / "type") == "bluetooth")
                settings.emplace("bluetooth/" + name, Setting{entry.path() / "soft", "1"});
        }
    }
    return settings;
}
bool validValue(const std::string &key, const std::string &value, const Setting &setting)
{
    if (key.rfind("cpu/", 0) == 0) {
        std::istringstream options(read(setting.path.parent_path() / "scaling_available_governors"));
        std::string option;
        while (options >> option) if (option == value) return true;
        return false;
    }
    if (key.rfind("nvme/", 0) == 0) return value == "auto" || value == "on";
    return value == "0" || value == "1";
}
void restore(const Paths &paths)
{
    if (!fs::exists(paths.state / "settings")) return;
    const auto current = candidates(paths, "all");
    std::ifstream saved(paths.state / "settings");
    if (!saved) throw std::runtime_error("Cannot read low-power recovery state");
    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(saved, line)) {
        std::istringstream fields(line);
        std::string key, value, extra;
        if (!(fields >> key >> value) || (fields >> extra))
            throw std::runtime_error("Malformed low-power recovery state");
        if (!values.emplace(key, value).second)
            throw std::runtime_error("Duplicate low-power recovery key");
        const auto found = current.find(key);
        if (found != current.end() && !validValue(key, value, found->second))
            throw std::runtime_error("Invalid low-power recovery value");
        if (!numbered(key, "cpu/policy") && !numbered(key, "nvme/nvme") &&
            !numbered(key, "bluetooth/rfkill"))
            throw std::runtime_error("Invalid low-power recovery key");
    }
    if (saved.bad()) throw std::runtime_error("Failed reading low-power recovery state");
    bool failed = false;
    for (const auto &[name, original] : values) {
        const auto found = current.find(name);
        if (found == current.end()) continue; // A removable device may have left.
        try {
            write(found->second.path, original);
            std::cout << "restored " << name << '=' << original << '\n';
        } catch (const std::exception &error) {
            failed = true;
            std::cerr << error.what() << '\n';
        }
    }
    if (failed) throw std::runtime_error("Low-power restore incomplete; recovery state retained");
    fs::remove(paths.state / "settings");
}
void enter(const Paths &paths, const std::string &mode)
{
    if (fs::exists(paths.state / "settings")) restore(paths);
    const auto settings = candidates(paths, mode);
    std::ofstream saved(paths.state / "settings.tmp");
    for (const auto &[name, setting] : settings) {
        const auto original = read(setting.path);
        if (!validValue(name, original, setting)) throw std::runtime_error("Unexpected setting: " + name);
        saved << name << ' ' << original << '\n';
    }
    saved.flush();
    if (!saved) throw std::runtime_error("Cannot save low-power recovery state");
    saved.close();
    fs::rename(paths.state / "settings.tmp", paths.state / "settings");
    try {
        for (const auto &[name, setting] : settings) {
            write(setting.path, setting.target);
            std::cout << "applied " << name << '=' << setting.target << '\n';
        }
    } catch (...) { restore(paths); throw; }
}
}

#ifndef APERTAR_LOW_POWER_TEST
int main(int argc, char **argv)
{
    const std::string mode = argc == 2 ? argv[1] : "";
    if (mode != "cpu" && mode != "nvme" && mode != "bluetooth" && mode != "all" && mode != "restore") {
        std::cerr << "Usage: apertar-standby-low-power cpu|nvme|bluetooth|all|restore\n";
        return 1;
    }
    int lock = -1;
    try {
        if (::geteuid() != 0) throw std::runtime_error("Low-power control requires the privileged helper");
        lowPower::Paths paths;
        if (::mkdir(paths.state.c_str(), 0755) != 0 && errno != EEXIST)
            throw std::runtime_error("Cannot create recovery directory");
        struct stat info {};
        if (::lstat(paths.state.c_str(), &info) != 0 || !S_ISDIR(info.st_mode) ||
            info.st_uid != 0 || (info.st_mode & 0022))
            throw std::runtime_error("Unsafe low-power recovery directory");
        lock = ::open((paths.state / "lock").c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (lock < 0 || ::flock(lock, LOCK_EX | LOCK_NB) != 0)
            throw std::runtime_error("Another low-power operation is running");
        if (mode == "restore") lowPower::restore(paths);
        else lowPower::enter(paths, mode);
        ::close(lock);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        if (lock >= 0) ::close(lock);
        return 1;
    }
}
#endif
