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
    static SocketService& Instance() {
        static SocketService _instance;
        return _instance;
    }

    int SendData(const char* data, int size) {
        FUNC_WARDER;

        if (data == NULL || size == 0 || m_socket == INVALID_SOCKET || m_stop) {
            return 0;
        }

        LOG("SendData %s", data);

        int bytes = 0;
        m_synchronizer.Lock();
        for (int i = 0; i < 2 && size - bytes > 0;) {
            int ret = send(m_socket, data + bytes, size - bytes, 0);
            if (ret == SOCKET_ERROR) {
                Init();
                ++i;
            } else {
                bytes += ret;
            }
        }
        m_synchronizer.Unlock();
        InterlockedExchange(&m_last_send_time, time(NULL));
        return bytes;
    }

    void OnConfigChanged(const char* server, int port) {
        if (strcmp(m_server, server) == 0 && m_port == port) {
            return;
        }
        COPY_STR(m_server, server);
        m_port = port;
        Init();
    }

    void Stop() {
        m_stop = true;
        WaitForSingleObject(m_thread, INFINITE);
    }

private:
    static void HeartBeat(void* self) {
        ((SocketService*)self)->HeartBeat();
    }

    void HeartBeat() {
        FUNC_WARDER;

        while (!m_stop) {
            int t = time(NULL);
            if (t - m_last_send_time > 5) {
                SendData(" ", 1);
            }
            // for fast exit and reduce frequently calling time(NULL)
            for (int i = 0; i < 100; i++) {
                if (m_stop) {
                    break;
                }
                Sleep(50L);
            }
        }
        _endthread();
    }

    bool Lookup(SOCKADDR_IN* socket_addr) {
        struct addrinfo *answer = NULL, hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        if (0 == getaddrinfo(m_server, NULL, &hint, &answer)) {
            if (answer->ai_family == AF_INET) {
                memcpy(socket_addr, answer->ai_addr, sizeof(SOCKADDR_IN));
                freeaddrinfo(answer);
                return true;
            }
        }
        return false;
    }

    void Init() {
        FUNC_WARDER;

        m_synchronizer.Lock();
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
        }

        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET) {
            LOG("invalid socket");
            m_synchronizer.Unlock();
            return;
        }

        sockaddr_in server_addr;
        if (!Lookup(&server_addr)) {
            LOG("invalid server name");
            m_synchronizer.Unlock();
            return;
        }
        server_addr.sin_port = htons(m_port);
        if (connect(m_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            LOG("connect error @%H, %H ", server_addr.sin_port, server_addr.sin_addr);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
        m_synchronizer.Unlock();
    }

    SocketService() : m_socket(INVALID_SOCKET), m_last_send_time(0), m_stop(false) {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            LOG("WSAStartup fails");
        }
        m_thread = (HANDLE)_beginthread(HeartBeat, 0, this);
    };

    SocketService(const SocketService& post) = delete;

    void operator=(const SocketService& post) = delete;

    ~SocketService() {
        WSACleanup();
    };

private:
    SOCKET m_socket;
    char m_server[256];
    int m_port;
    Synchronizer m_synchronizer;
    long m_last_send_time;
    bool m_stop;
    HANDLE m_thread;
};

#endif  // !_SOCKETSERVICE_H_
