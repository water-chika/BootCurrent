#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <filesystem>
#include <fstream>

#include <win32_helper.hpp>

#define EFI_GLOBAL_VARIABLE "{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}"

void ObtainPrivileges(const char *privilege)
{
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    {
        throw std::exception("OpenProcessToken fail");
    }
    TOKEN_PRIVILEGES tp = {0};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValueA(NULL, privilege, &tp.Privileges[0].Luid))
    {
        throw std::exception("LookupPrivilegeValue fail");
    }
    if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
    {
        throw std::exception("AdjustTokenPrivileges fail");
    }
    else
    {
        if (GetLastError() != ERROR_SUCCESS)
        {
            throw std::exception("AdjustTokenPrivileges not adjust");
        }
    }
}

using bootnum_t = uint16_t;

std::unordered_map<std::string, bootnum_t> parse_config(std::istream& in) {
    std::unordered_map<std::string, bootnum_t> boot_name_num_map{};
    
    {
        std::string name{};
        bootnum_t bootnum{};
        while (!in.eof()) {
            in >> name >> bootnum;
            boot_name_num_map.emplace(std::move(name), bootnum);
        }
    }

    return boot_name_num_map;
}

std::unordered_map<std::string, bootnum_t> parse_config(const std::filesystem::path& path) {
    auto in = std::ifstream{ path };
    return parse_config(in);
}

std::unordered_map<std::string, bootnum_t> parse_config() {
    auto module_path = win32_helper::get_module_file_name();

    auto directory = std::filesystem::path{module_path.data()}.parent_path();
    auto config_path = directory / "boot.cfg";

    auto config = parse_config(config_path);
    return config;
}

void boot_current() {    
    ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME);
    //ObtainPrivileges(SE_SHUTDOWN_NAME);
    char buffer[1024];
    bootnum_t boot_current = 0;
    if (!GetFirmwareEnvironmentVariableA("BootCurrent", EFI_GLOBAL_VARIABLE, &boot_current, sizeof(boot_current)))
    {
        throw std::exception("GetFirmwareEnvironmentVariable fail");
    }
    std::cout << "BootCurrent:" << boot_current << std::endl;
    int size = 0;
    if (! (size = GetFirmwareEnvironmentVariableA("BootOrder", EFI_GLOBAL_VARIABLE, buffer, sizeof(buffer)))) {
        throw std::exception("GetFirmwareEnvironmentVariable fail");
    }
    std::cout << "BootOrder:";
    for (int i = 0; i < size/sizeof(bootnum_t); i++) {
        std::cout << ((bootnum_t*)buffer)[i] << ',';
    }
    std::cout << std::endl;

    std::cout << "Try to get current BootNext" << std::endl;
    bootnum_t bootnext = 0;
    if (!GetFirmwareEnvironmentVariable("BootNext", EFI_GLOBAL_VARIABLE, &bootnext, sizeof(bootnext)))
    {
        std::cout << "get bootnext fail" << std::endl;
    }
    else
    {
        std::cout << "BootNext:" << bootnext << std::endl;
    }
    if (bootnext == boot_current) {
        std::cout << "BootNext equal to BootCurrent, will quit!" << std::endl;
        return;
    }
    std::cout << "Try to set BootNext" << std::endl;
    bootnext = boot_current;
    if (!SetFirmwareEnvironmentVariableA("BootNext", EFI_GLOBAL_VARIABLE, &bootnext, sizeof(bootnum_t)))
    {
        throw std::exception("SetFirmwareEnvironmentVariable fail");
    }
    std::cout << "After set BootNext" << std::endl;
    if (!GetFirmwareEnvironmentVariableA("BootNext", EFI_GLOBAL_VARIABLE, buffer, sizeof(buffer)))
    {
        throw std::exception("GetFirmwareEnvironmentVariable fail");
    }
    std::cout << "BootNext:" << *(bootnum_t *)buffer << std::endl;
}



void boot_to(auto name) {
    ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME);
    //ObtainPrivileges(SE_SHUTDOWN_NAME);

    auto config = parse_config();
    auto bootnum = config[name];
    std::cout << bootnum << std::endl;

    bootnum_t bootnext = bootnum;
    if (!SetFirmwareEnvironmentVariableA("BootNext", EFI_GLOBAL_VARIABLE, &bootnext, sizeof(uint16_t)))
    {
        throw std::exception("SetFirmwareEnvironmentVariable fail");
    }
}

int main(int argc, const char** argv)
{
    try
    {
        if (argc <= 1) {
            boot_current();
        }
        else if (argc == 2) {
            boot_to(argv[1]);
        }
        else {
            std::cerr << std::format("Usage: {} <name>", argv[0]) << std::endl;
            return -1;
        }
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << ":";
        DWORD errCode = GetLastError();
        char *lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        std::cout << lpMsgBuf << std::endl;
    }
    return 0;
}