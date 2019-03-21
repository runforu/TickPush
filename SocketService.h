#ifndef _SOCKETSERVICE_H_
#define _SOCKETSERVICE_H_

#include <Ws2tcpip.h>
#include <iostream>
#include <process.h>
#include <time.h>
#include <winsock2.h>
#include "Loger.h"
#include "Synchronizer.h"
#include "common.h"

class SocketService {
public:
    static SocketService& Instance();

    int SendData(const char* data, int size);

    void OnConfigChanged(const char* server, int port);

    void Stop();

private:
    static void HeartBeat(void* self);

    void HeartBeat();

    bool Lookup(SOCKADDR_IN* socket_addr);

    void Init();

    SocketService();

    SocketService(const SocketService& post) = delete;

    void operator=(const SocketService& post) = delete;

    ~SocketService();

private:
    SOCKET m_socket;
    char m_server[256];
    int m_port;
    Synchronizer m_synchronizer;
    long m_last_sent_time;
    bool m_stop;
    HANDLE m_thread;
};

#endif  // !_SOCKETSERVICE_H_
