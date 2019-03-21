#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#include "common.h"

#define MAX_GROUPS 16

class Processor {
public:
    static Processor& Instance();

    void ShowStatus();

    void TickApply(const ConSymbol* symbol, FeedTick* inf);

    void Initialize();

    void Shutdown(void);

private:
    Processor();
    Processor(const Processor&) = delete;
    void operator=(const Processor&) = delete;

    inline int GetSpreadDiff(const char* group, const ConSymbol* con_symbol);

    inline void SpreadDiff(const char* group, const ConSymbol* con_symbol, double* bid, double* ask);

private:
    //--- configurations
    int m_disable_plugin;
    char m_notice_server[128];
    char m_groups_string[128];
    int m_port;

    char m_groups[MAX_GROUPS][16];
    int m_group_count;
};

//+------------------------------------------------------------------+
#endif  // !_PROCESSOR_H_
