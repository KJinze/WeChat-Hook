// dllmain.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

#include "global.h"
#include "hideself.h"
#include "tools.h"
#include "version_proxy.h"

int g_receive_type = 1;
int g_StartPort = 30001;
int g_MsgSendPort = 80;

static void ParseCmdLine(const std::string& cmd)
{
    const std::string key = "StartPort=";
    size_t pos = cmd.find(key);
    if (pos != std::string::npos)
    {
        pos += key.length();

        int port = 0;
        while (pos < cmd.length())
        {
            char ch = cmd[pos];
            if (ch < '0' || ch > '9')
                break;

            port = port * 10 + (ch - '0');
            pos++;
        }

        if (port > 0 && port <= 65535)
            g_StartPort = port;
    }

    const std::string recvTypeKey = "RecvType=";
    size_t recvTypePos = cmd.find(recvTypeKey);
    if (recvTypePos != std::string::npos)
    {
        recvTypePos += recvTypeKey.length();

        int receiveType = 0;
        while (recvTypePos < cmd.length())
        {
            char ch = cmd[recvTypePos];
            if (ch < '0' || ch > '9')
                break;

            receiveType = receiveType * 10 + (ch - '0');
            recvTypePos++;
        }

        if (receiveType > 0 && receiveType <= 2)
            g_receive_type = receiveType;
    }

    const std::string callbackKey = "CallBackURL=";
    size_t callbackPos = cmd.find(callbackKey);
    if (callbackPos != std::string::npos)
    {
        callbackPos += callbackKey.length();

        std::string url = ExtractQuotedString(cmd, callbackPos);
        if (!url.empty())
        {
            g_CallBack_Url = url;
            g_MsgSendPort = 80;

            size_t schemeEnd = url.find("://");
            size_t hostStart = (schemeEnd == std::string::npos) ? 0 : schemeEnd + 3;
            size_t pathPos = url.find('/', hostStart);
            std::string hostPort = (pathPos == std::string::npos)
                ? url.substr(hostStart)
                : url.substr(hostStart, pathPos - hostStart);

            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos)
            {
                std::string portStr = hostPort.substr(colonPos + 1);
                int port = atoi(portStr.c_str());
                if (port > 0 && port <= 65535)
                    g_MsgSendPort = port;
            }
        }
    }
}

static std::string GetCmdLine()
{
    std::wstring wcmd = GetCommandLineW();
    return WStringToString(wcmd);
}

static bool IsMainWeixinProcess()
{
    std::string cmd = GetCmdLine();

    if (cmd.find("--type=") != std::string::npos)
        return false;

    if (cmd.find("--crashpad-handler") != std::string::npos)
        return false;

    ParseCmdLine(cmd);
    return true;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;

        DisableThreadLibraryCalls(hModule);
        InitRealDll();

        if (!IsMainWeixinProcess())
            return TRUE;

        HideModuleFromPEB(hModule);
#ifdef _DEBUG
        OutputDebugStringA("[VxHook] HideModuleFromPEB success!\n");
#endif
        CustomInit(hModule);
    }

    return TRUE;
}
