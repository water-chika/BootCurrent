#include <windows.h>
#include <iostream>
#include <cstdlib>

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

int main()
{
    try
    {
        ObtainPrivileges(SE_SYSTEM_ENVIRONMENT_NAME);
        //ObtainPrivileges(SE_SHUTDOWN_NAME);
        char buffer[1024];
        uint16_t boot_current = 0;
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
        for (int i = 0; i < size/sizeof(uint16_t); i++) {
            std::cout << ((uint16_t*)buffer)[i] << ',';
        }
        std::cout << std::endl;

        std::cout << "Try to get current BootNext" << std::endl;
        uint16_t bootnext = 0;
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
            return 0;
        }
        std::cout << "Try to set BootNext" << std::endl;
        bootnext = boot_current;
        if (!SetFirmwareEnvironmentVariableA("BootNext", EFI_GLOBAL_VARIABLE, &bootnext, sizeof(uint16_t)))
        {
            throw std::exception("SetFirmwareEnvironmentVariable fail");
        }
        std::cout << "After set BootNext" << std::endl;
        if (!GetFirmwareEnvironmentVariableA("BootNext", EFI_GLOBAL_VARIABLE, buffer, sizeof(buffer)))
        {
            throw std::exception("GetFirmwareEnvironmentVariable fail");
        }
        std::cout << "BootNext:" << *(uint16_t *)buffer << std::endl;
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