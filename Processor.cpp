#include <process.h>
#include <stdio.h>
#include <winsock2.h>
#include "Config.h"
#include "Loger.h"
#include "Processor.h"
#include "ServerApi.h"
#include "SocketService.h"
#include "LicenseService.h"

void Processor::Shutdown(void) {
    SocketService::Instance().Stop();

#ifdef _LICENSE_VERIFICATION_
    LicenseService::Instance().Stop();
#endif  // !_LICENSE_VERIFICATION_

    ShowStatus();
}

Processor::Processor() : m_disable_plugin(0), m_group_count(0), m_port(0) {}

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
    Config::Instance().GetString("Group_0", m_groups_string[0], sizeof(m_groups_string[0]) - 1, "");
    Config::Instance().GetString("Group_1", m_groups_string[1], sizeof(m_groups_string[1]) - 1, "");
    Config::Instance().GetString("Group_2", m_groups_string[2], sizeof(m_groups_string[2]) - 1, "");
    Config::Instance().GetString("Group_3", m_groups_string[3], sizeof(m_groups_string[3]) - 1, "");
    Config::Instance().GetInteger("Port", &m_port, "8888");

    m_group_count = 0;
    memset(m_groups, 0, sizeof(m_groups));

    for (int i = 0; i < MAX_GROUP_SETTINTS; ++i) {
        char* context = NULL;
        char* p = strtok_s(m_groups_string[i], "|", &context);
        while (m_group_count < MAX_SEC_GROUPS && p != NULL) {
            int position = 0;
            for (; position < m_group_count; ++position) {
                if (strncmp(p, m_groups[position], sizeof(m_groups[position])) == 0) {
                    break;
                }
            }
            if (position == m_group_count) {
                COPY_STR(m_groups[m_group_count], p);
                LOG("group %d: %s", m_group_count, m_groups[m_group_count]);
                ++m_group_count;
            }
            p = strtok_s(NULL, "|", &context);
        }
    }

    SocketService::Instance().OnConfigChanged(m_notice_server, m_port);

#ifdef _LICENSE_VERIFICATION_
    LicenseService::Instance().ResetLicense();
#endif  // !_LICENSE_VERIFICATION_
}

void Processor::TickApply(const ConSymbol* symbol, FeedTick* inf) {
#ifdef _LICENSE_VERIFICATION_
    if (!LicenseService::Instance().IsLicenseValid()) {
        return;
    }
#endif  // !_LICENSE_VERIFICATION_

    if (m_disable_plugin) {
        return;
    }

    if (m_group_count == 0) {
        return;
    }

    char buf[1024] = {0};
    for (int i = 0; i < m_group_count; i++) {
        double bid = inf->bid;
        double ask = inf->ask;
        if (!SpreadDiff(m_groups[i], symbol, &bid, &ask)) {
            continue;
        }
        int len = strlen(buf);
        sprintf_s(buf + len, sizeof(buf) - len - 1, "%s,%s,%d,%f,%f>", inf->symbol, m_groups[i], inf->ctm, bid, ask);
    }

    SocketService::Instance().SendData(buf, strlen(buf));
}

inline bool Processor::SpreadDiff(const char* group, const ConSymbol* con_symbol, double* bid, double* ask) {
    ConGroup con_group;
    ServerApi::Api()->GroupsGet(group, &con_group);
    if (con_group.secgroups[con_symbol->type].show == 0) {
        return false;
    }
    int diff = con_group.secgroups[con_symbol->type].spread_diff;
    if (diff != 0) {
        *bid = NormalizeDouble(*bid - con_symbol->point * diff / 2, con_symbol->digits);
        *ask = NormalizeDouble(*ask + con_symbol->point * diff / 2, con_symbol->digits);
    }
    return true;
}
