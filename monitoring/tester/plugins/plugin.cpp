#include "iplugin.h"
#include "plugin.h"
#include "log.h"

static const char* _tag = "[" PLUGIN_NAME "] ";

extern IPluginApi* pluginApi(PLUGIN_NAME);

//
// _cdecl Bind file logger
//
extern "C" int BindFileLogger() {
    return 0;
}

//
// _cdecl Stop execute plugin
//
extern "C" int Stop(uint32_t id) {
    LOG_TRACE << _tag << "Arrived stop..";
    if(pluginApi(PLUGIN_NAME) != nullptr) {
        return pluginApi(PLUGIN_NAME)->Stop(id);
    }
    return -1;
}

//
// _cdecl Start execute plugin
//
extern "C" int Execute(uint32_t id, uint32_t delay) {
    LOG_TRACE << _tag << "Arrived execute..";
    if(pluginApi(PLUGIN_NAME) != nullptr) {
        return pluginApi(PLUGIN_NAME)->Execute(id, delay);
    }
    return -1;
}

//
// _cdecl Get report
extern "C" int GetReport(uint32_t id, void** report) {
    LOG_TRACE << _tag << "Arrived get report..";
    if(pluginApi(PLUGIN_NAME) != nullptr) {
        return pluginApi(PLUGIN_NAME)->GetReport(id, report);
    }
    return -1;
}

//
// _cdecl Unregister worker in plugin
//
extern "C" int UnRegisterWorker(uint32_t id) {
    LOG_TRACE << _tag << "Arrived worker unregistering..";
    if(pluginApi(PLUGIN_NAME) != nullptr) {
        return pluginApi(PLUGIN_NAME)->UnRegisterWorker(id);
    }
    return -1;
}


//
// _cdecl Register worker in plugin
//
extern "C" int RegisterWorker(uint32_t id, IWorker* worker) {
    LOG_TRACE << _tag << "Arrived worker registering..";
    if(pluginApi(PLUGIN_NAME) != nullptr) {
        return pluginApi(PLUGIN_NAME)->RegisterWorker(id, worker);
    }
    return -1;
}

//
// _cdecl Send extended data in plugin
//
extern "C" int SendExtendData(uint32_t id, char* data, int length) {
    LOG_TRACE << _tag << "Arrived extend data..";
    if(pluginApi(PLUGIN_NAME) != nullptr) {
        return pluginApi(PLUGIN_NAME)->SendExtendData(id, data, length);
    }
    return -1;
}


//
// _cdecl Get plugin type
//
extern "C" int GetPluginType() {
    return PLUGIN_ID;
}

//
// _cdecl Get plugin string name
//
extern "C" const char* GetPluginName() {
    return PLUGIN_NAME;
}


