#include "Config.h"
#include "Loger.h"
#include "Processor.h"
#include "ServerApi.h"

extern const char *PLUGIN_VERSION_STRING;
#define PLUGIN_NAME "Tick Push"
#define PLUGIN_VERSION 1
#define PLUGIN_COPYRIGHT "DH Copyrigh."

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            char tmp[256], *cp;
            //--- create configuration filename
            GetModuleFileName((HMODULE)hModule, tmp, sizeof(tmp) - 5);
            if ((cp = strrchr(tmp, '.')) != NULL) {
                *cp = 0;
                strcat(tmp, ".ini");
            }
            Config::Instance().Load(tmp);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return (TRUE);
}

void APIENTRY MtSrvAbout(PluginInfo* info) {
    if (info != NULL) {
        sprintf(info->name, "%s %s", PLUGIN_NAME, PLUGIN_VERSION_STRING);
        info->version = PLUGIN_VERSION;
        sprintf(info->copyright, "%s", PLUGIN_COPYRIGHT);
        memset(info->reserved, 0, sizeof(info->reserved));
    }
}

int APIENTRY MtSrvStartup(CServerInterface* server) {
    if (server == NULL) {
        return (FALSE);
    }
    //--- check version
    if (server->Version() != ServerApiVersion) {
        return (FALSE);
    }
    //--- save server interface link
    ServerApi::Initialize(server);

    //--- initialize dealer helper
    Processor::Instance().Initialize();

    return (TRUE);
}

void APIENTRY MtSrvCleanup() {
    Processor::Instance().Shutdown();
}

int APIENTRY MtSrvPluginCfgSet(const PluginCfg* values, const int total) {
    LOG("MtSrvPluginCfgSet total = %d.", total);
    int res = Config::Instance().Set(values, total);
    Processor::Instance().Initialize();
    return (res);
}

int APIENTRY MtSrvPluginCfgNext(const int index, PluginCfg* cfg) {
    LOG("MtSrvPluginCfgNext index=%d, name=%s, value=%s.", index, cfg->name, cfg->value);
    return Config::Instance().Next(index, cfg);
}

int APIENTRY MtSrvPluginCfgTotal() {
    LOG("MtSrvPluginCfgTotal.");
    return Config::Instance().Total();
}

void APIENTRY MtSrvHistoryTickApply(const ConSymbol* symbol, FeedTick* inf) {
    Processor::Instance().TickApply(symbol, inf);
}