#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdlib>

namespace forma::toolchain {

struct ToolchainInfo {
    std::string name;
    std::string download_url;
    std::string archive_name;
    std::string bin_dir;  // Relative path to bin directory after extraction
    std::string compiler_name;  // e.g., "gcc", "arm-none-eabi-gcc"
    std::string description;
};

class ToolchainManager {
public:
    // Map of target to toolchain download info
    static std::map<std::string, ToolchainInfo> get_toolchain_database() {
        std::map<std::string, ToolchainInfo> db;
        
        // x86_64 Linux (native)
        db["x86_64-linux-gnu"] = {
            "GCC 13.2.0 x86_64",
            "https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz",
            "gcc-x86_64-linux-gnu.tar.xz",
            "bin",
            "gcc",
            "Native x86_64 Linux GCC compiler"
        };
        
        // ARM64/AArch64 Linux cross-compiler
        db["aarch64-linux-gnu"] = {
            "GCC 13.2.0 AArch64",
            "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz",
            "gcc-aarch64-linux-gnu.tar.xz",
            "bin",
            "aarch64-none-linux-gnu-gcc",
            "ARM 64-bit Linux cross-compiler"
        };
        
        // ARM 32-bit Linux cross-compiler
        db["arm-linux-gnueabihf"] = {
            "GCC 13.2.0 ARM",
            "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz",
            "gcc-arm-linux-gnueabihf.tar.xz",
            "bin",
            "arm-none-linux-gnueabihf-gcc",
            "ARM 32-bit Linux cross-compiler"
        };
        
        // STM32 ARM Cortex-M (bare-metal)
        db["arm-none-eabi"] = {
            "GCC 13.2.0 ARM Cortex-M",
            "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz",
            "gcc-arm-none-eabi.tar.xz",
            "bin",
            "arm-none-eabi-gcc",
            "ARM Cortex-M bare-metal (STM32, etc.)"
        };
        
        // STM32 (alias for arm-none-eabi)
        db["stm32"] = {
            "GCC 13.2.0 ARM Cortex-M (STM32)",
            "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz",
            "gcc-arm-none-eabi.tar.xz",
            "bin",
            "arm-none-eabi-gcc",
            "STM32 ARM Cortex-M microcontrollers"
        };
        
        // ESP32 Xtensa (managed by ESP-IDF)
        db["esp32"] = {
            "ESP32 Xtensa Toolchain",
            "https://github.com/espressif/crosstool-NG/releases/download/esp-12.2.0_20230208/xtensa-esp32-elf-12.2.0_20230208-x86_64-linux-gnu.tar.xz",
            "xtensa-esp32-elf.tar.xz",
            "xtensa-esp32-elf/bin",
            "xtensa-esp32-elf-gcc",
            "ESP32 Xtensa LX6 toolchain (WiFi + BT Classic + BLE)"
        };
        
        // ESP32-S2 Xtensa
        db["esp32s2"] = {
            "ESP32-S2 Xtensa Toolchain",
            "https://github.com/espressif/crosstool-NG/releases/download/esp-12.2.0_20230208/xtensa-esp32s2-elf-12.2.0_20230208-x86_64-linux-gnu.tar.xz",
            "xtensa-esp32s2-elf.tar.xz",
            "xtensa-esp32s2-elf/bin",
            "xtensa-esp32s2-elf-gcc",
            "ESP32-S2 Xtensa LX7 toolchain (WiFi + USB OTG)"
        };
        
        // ESP32-S3 Xtensa
        db["esp32s3"] = {
            "ESP32-S3 Xtensa Toolchain",
            "https://github.com/espressif/crosstool-NG/releases/download/esp-12.2.0_20230208/xtensa-esp32s3-elf-12.2.0_20230208-x86_64-linux-gnu.tar.xz",
            "xtensa-esp32s3-elf.tar.xz",
            "xtensa-esp32s3-elf/bin",
            "xtensa-esp32s3-elf-gcc",
            "ESP32-S3 Xtensa LX7 toolchain (WiFi + BLE + USB OTG + AI)"
        };
        
        // ESP32-C3 RISC-V
        db["esp32c3"] = {
            "ESP32-C3 RISC-V Toolchain",
            "https://github.com/espressif/crosstool-NG/releases/download/esp-12.2.0_20230208/riscv32-esp-elf-12.2.0_20230208-x86_64-linux-gnu.tar.xz",
            "riscv32-esp-elf.tar.xz",
            "riscv32-esp-elf/bin",
            "riscv32-esp-elf-gcc",
            "ESP32-C3 RISC-V toolchain (WiFi + BLE + low power)"
        };
        
        // ESP32-C6 RISC-V
        db["esp32c6"] = {
            "ESP32-C6 RISC-V Toolchain",
            "https://github.com/espressif/crosstool-NG/releases/download/esp-12.2.0_20230208/riscv32-esp-elf-12.2.0_20230208-x86_64-linux-gnu.tar.xz",
            "riscv32-esp-elf.tar.xz",
            "riscv32-esp-elf/bin",
            "riscv32-esp-elf-gcc",
            "ESP32-C6 RISC-V toolchain (WiFi 6 + BLE 5 + Zigbee + Thread)"
        };
        
        // Windows cross-compiler (MinGW-w64)
        db["x86_64-w64-mingw32"] = {
            "MinGW-w64 GCC 13.2.0",
            "https://github.com/niXman/mingw-builds-binaries/releases/download/13.2.0-rt_v11-rev0/x86_64-13.2.0-release-posix-seh-msvcrt-rt_v11-rev0.7z",
            "mingw-w64-x86_64.7z",
            "mingw64/bin",
            "x86_64-w64-mingw32-gcc",
            "Windows 64-bit cross-compiler (MinGW-w64)"
        };
        
        // RISC-V 64-bit
        db["riscv64-linux-gnu"] = {
            "GCC 13.2.0 RISC-V",
            "https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2023.11.08/riscv64-glibc-ubuntu-22.04-gcc-nightly-2023.11.08-nightly.tar.gz",
            "gcc-riscv64-linux-gnu.tar.gz",
            "bin",
            "riscv64-unknown-linux-gnu-gcc",
            "RISC-V 64-bit Linux cross-compiler"
        };
        
        return db;
    }
    
    // Check if a compiler for the target is available
    static bool is_compiler_available(const std::string& target) {
        auto db = get_toolchain_database();
        auto it = db.find(target);
        if (it == db.end()) {
            return false;
        }
        
        std::string compiler = it->second.compiler_name;
        std::string check_cmd = compiler + " --version > /dev/null 2>&1";
        return system(check_cmd.c_str()) == 0;
    }
    
    // Download and install toolchain for a specific target
    static bool download_and_install(const std::string& target, const std::string& install_base) {
        auto db = get_toolchain_database();
        auto it = db.find(target);
        if (it == db.end()) {
            return false; // Unknown target
        }
        
        const ToolchainInfo& info = it->second;
        std::string install_dir = install_base + "/" + target;
        
        // Create install directory
        std::string cmd = "mkdir -p \"" + install_dir + "\"";
        if (system(cmd.c_str()) != 0) {
            return false;
        }
        
        // Download
        std::string archive_path = install_dir + "/" + info.archive_name;
        cmd = "curl -L -o \"" + archive_path + "\" \"" + info.download_url + "\"";
        if (system(cmd.c_str()) != 0) {
            // Try wget as fallback
            cmd = "wget -O \"" + archive_path + "\" \"" + info.download_url + "\"";
            if (system(cmd.c_str()) != 0) {
                return false;
            }
        }
        
        // Extract based on file extension
        if (info.archive_name.find(".tar.xz") != std::string::npos) {
            cmd = "cd \"" + install_dir + "\" && tar xJf \"" + info.archive_name + "\" --strip-components=1";
        } else if (info.archive_name.find(".tar.gz") != std::string::npos) {
            cmd = "cd \"" + install_dir + "\" && tar xzf \"" + info.archive_name + "\" --strip-components=1";
        } else if (info.archive_name.find(".7z") != std::string::npos) {
            cmd = "cd \"" + install_dir + "\" && 7z x -y \"" + info.archive_name + "\"";
        } else if (info.archive_name.find(".zip") != std::string::npos) {
            cmd = "cd \"" + install_dir + "\" && unzip -q \"" + info.archive_name + "\"";
        } else {
            return false; // Unknown archive format
        }
        
        if (system(cmd.c_str()) != 0) {
            return false;
        }
        
        // Clean up archive
        cmd = "rm \"" + archive_path + "\"";
        system(cmd.c_str());
        
        return true;
    }
    
    // Get the path to the compiler for a target
    static std::string get_compiler_path(const std::string& target, const std::string& install_base) {
        auto db = get_toolchain_database();
        auto it = db.find(target);
        if (it == db.end()) {
            return "";
        }
        
        const ToolchainInfo& info = it->second;
        return install_base + "/" + target + "/" + info.bin_dir + "/" + info.compiler_name;
    }
    
    // Ensure compiler for target is available, downloading if necessary
    // Returns the full path to the compiler (or just the compiler name if on PATH)
    static std::string ensure_compiler_available(const std::string& target) {
        // First check if compiler is on PATH
        if (is_compiler_available(target)) {
            auto db = get_toolchain_database();
            auto it = db.find(target);
            if (it != db.end()) {
                return it->second.compiler_name; // Use system compiler
            }
        }
        
        // Get installation base directory
        std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
#if defined(_WIN32)
        home = std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : "";
#endif
        
        if (home.empty()) {
            return ""; // Can't determine home directory
        }
        
        std::string toolchain_dir = home + "/.forma/toolchains";
        std::string compiler_path = get_compiler_path(target, toolchain_dir);
        
        // Check if downloaded compiler exists
        std::string check_cmd = "\"" + compiler_path + "\" --version > /dev/null 2>&1";
        if (system(check_cmd.c_str()) == 0) {
            return compiler_path; // Use downloaded compiler
        }
        
        // Need to download
        if (!download_and_install(target, toolchain_dir)) {
            return ""; // Download failed
        }
        
        return compiler_path;
    }
    
    // Get list of supported targets
    static std::vector<std::string> get_supported_targets() {
        auto db = get_toolchain_database();
        std::vector<std::string> targets;
        for (const auto& pair : db) {
            targets.push_back(pair.first);
        }
        return targets;
    }
    
    // Get toolchain info for a target
    static ToolchainInfo get_toolchain_info(const std::string& target) {
        auto db = get_toolchain_database();
        auto it = db.find(target);
        if (it != db.end()) {
            return it->second;
        }
        return ToolchainInfo{};
    }
};

} // namespace forma::toolchain
