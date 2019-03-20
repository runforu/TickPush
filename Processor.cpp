#include <process.h>
#include <stdio.h>
#include <winsock2.h>
#include "Config.h"
#include "Loger.h"
#include "Processor.h"
#include "ServerApi.h"
#include "SocketService.h"

void Processor::Shutdown(void) {
    SocketService::Instance().Stop();
    ShowStatus();
}

Processor::Processor() : m_reinitialize_flag(0), m_disable_plugin(0), m_group_count(0), m_port(220) {
    LOG("Processor::Processor   port =%d", m_port);
}

Processor& Processor::Instance() {
    static Processor _instance;
    return _instance;
}

void Processor::ShowStatus() {
    LOG("TickPush is going to shutdown.");
}

void Processor::Initialize() {
    FUNC_WARDER;

    Config::Instance().GetInteger("Disable Plugin", &m_disable_plugin, "0");
    Config::Instance().GetString("Server", m_notice_server, sizeof(m_notice_server) - 1, "127.0.0.1");
    Config::Instance().GetString("Groups", m_groups_string, sizeof(m_groups_string) - 1, "");
    Config::Instance().GetInteger("Port", &m_port, "8888");

    char* context = NULL;
    char* p = strtok_s(m_groups_string, "|", &context);
    while (m_group_count < MAX_GROUPS && p != NULL) {
        COPY_STR(m_groups[m_group_count], p);
        LOG("group %d: %s", m_group_count, m_groups[m_group_count]);
        p = strtok_s(NULL, "|", &context);
        ++m_group_count;
    }

    SocketService::Instance().OnConfigChanged(m_notice_server, m_port);
}

void Processor::TickApply(const ConSymbol* symbol, FeedTick* inf) {
    if (InterlockedExchange(&m_reinitialize_flag, 0) != 0) {
        Initialize();
    }

    if (m_disable_plugin) {
        return;
    }

    if (m_group_count == 0) {
        return;
    }

    FeedTick tick;
    memcpy(&tick, inf, sizeof(FeedTick));

    char buf[1024] = {0};
    sprintf_s(buf, "{\"symbol\":\"%s\",\"ctm\":%d,\"bank\":\"%s\",\"feeder\":%d,\"quotes\":[", tick.symbol, tick.ctm, tick.bank,
              tick.feeder);
    for (int i = 0; i < m_group_count; i++) {
        SpreadDiff(m_groups[i], symbol, &tick);
        int len = strlen(buf);
        if (i != m_group_count - 1) {
            sprintf_s(buf + len, sizeof(buf) - len - 1, "{\"group\":\"%s\",\"bid\":%f,\"ask\":%f},", m_groups[i], tick.bid,
                      tick.ask);
        } else {
            sprintf_s(buf + len, sizeof(buf) - len - 1, "{\"group\":\"%s\",\"bid\":%f,\"ask\":%f}]}|", m_groups[i], tick.bid,
                      tick.ask);
        }
    }
    LOG(buf);
    SocketService::Instance().SendData(buf, strlen(buf));
}

inline int Processor::GetSpreadDiff(const char* group, const ConSymbol* con_symbol) {
    ConGroup con_group;
    ServerApi::Api()->GroupsGet(group, &con_group);
    return con_group.secgroups[con_symbol->type].spread_diff;
}

inline void Processor::SpreadDiff(const char* group, const ConSymbol* con_symbol, FeedTick* tick) {
    int diff = GetSpreadDiff(group, con_symbol);
    if (diff != 0) {
        tick->bid = NormalizeDouble(tick->bid - con_symbol->point * diff / 2, con_symbol->digits);
        tick->ask = NormalizeDouble(tick->ask + con_symbol->point * diff / 2, con_symbol->digits);
    }
}
