#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <string>

#include "global.h"
#include "inline_weixin_dll_load.h"
#include "version_proxy.h"

namespace {

HMODULE g_hRealDll = nullptr;

constexpr const char* kExportNames[8] = {
    "CreateNetworkManagerNoPB",
    "DestroyContextNoPB",
    "DestroyIlinkStreamContext",
    "DestroyLogManagerNoPB",
    "DestroyNetworkManagerNoPB",
    "GetContextNoPB",
    "GetIlinkStreamContext",
    "GetLogManagerNoPB",
};

std::wstring BuildRealDllPath()
{
    wchar_t modulePath[MAX_PATH]{};
    if (!GetModuleFileNameW(g_hModule, modulePath, MAX_PATH))
        return L"";

    wchar_t* slash = wcsrchr(modulePath, L'\\');
    if (!slash)
        return L"";

    *(slash + 1) = L'\0';

    std::wstring realPath = modulePath;
    realPath += L"ilink_wrapper_real.dll";
    return realPath;
}

extern "C" void MissingIlinkWrapperExport()
{
    OutputDebugStringA("[VxHook] ilink_wrapper export missing\n");
    TerminateProcess(GetCurrentProcess(), ERROR_PROC_NOT_FOUND);
}

static DWORD WINAPI WaitWeixinDllAndInitThread(LPVOID)
{
    for (int i = 0; i < 300; ++i)
    {
        if (GetModuleHandleW(L"Weixin.dll"))
        {
            Evt_WeixinLoad();
            return 0;
        }

        Sleep(200);
    }

    OutputDebugStringA("[VxHook] Wait Weixin.dll timeout\n");
    return 0;
}

} // namespace

extern "C" void* g_ilink_wrapper_exports[8] = {
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
    reinterpret_cast<void*>(&MissingIlinkWrapperExport),
};

void InitRealDll()
{
    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;

    const std::wstring realDllPath = BuildRealDllPath();
    if (realDllPath.empty())
    {
        OutputDebugStringA("[VxHook] failed to build ilink_wrapper_real.dll path\n");
        return;
    }

    g_hRealDll = LoadLibraryW(realDllPath.c_str());
    if (!g_hRealDll)
    {
        OutputDebugStringA("[VxHook] failed to load ilink_wrapper_real.dll\n");
        return;
    }

    for (size_t i = 0; i < _countof(kExportNames); ++i)
    {
        FARPROC proc = GetProcAddress(g_hRealDll, kExportNames[i]);
        if (!proc)
        {
            OutputDebugStringA("[VxHook] failed to resolve ilink_wrapper export\n");
            return;
        }

        g_ilink_wrapper_exports[i] = reinterpret_cast<void*>(proc);
    }
}

void CustomInit(HMODULE hModule)
{
    static LONG initStarted = 0;

    if (InterlockedCompareExchange(&initStarted, 1, 0) != 0)
        return;

    g_hWeixinExe = GetModuleHandleW(L"weixin.exe");
    if (!g_hWeixinExe)
        return;

    if (MH_Initialize() != MH_OK)
        return;

    HANDLE hThread = CreateThread(
        nullptr,
        0,
        WaitWeixinDllAndInitThread,
        nullptr,
        0,
        nullptr
    );

    if (hThread)
        CloseHandle(hThread);
}
