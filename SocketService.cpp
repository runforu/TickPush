#include "SocketService.h"

SocketService &SocketService::Instance() {
    static SocketService _instance;
    return _instance;
}

int SocketService::SendData(const char *data, int size) {
    if (data == NULL || size == 0 || m_stop) {
        return 0;
    }

    if (m_socket == INVALID_SOCKET) {
        return SOCKET_ERROR;
    }

    int bytes = 0;
    m_synchronizer.Lock();
    while (size - bytes > 0) {
        int ret = send(m_socket, data + bytes, size - bytes, 0);
        if (ret == SOCKET_ERROR) {
            bytes = SOCKET_ERROR;
            break;
        }
        bytes += ret;
    }
    m_synchronizer.Unlock();
    InterlockedExchange(&m_last_sent_time, (long)time(NULL));
    return bytes;
}

void SocketService::OnConfigChanged(const char *server, int port) {
    FUNC_WARDER;

    if (strcmp(m_server, server) == 0 && m_port == port) {
        return;
    }
    COPY_STR(m_server, server);
    m_port = port;
    Init();
}

void SocketService::Stop() {
    FUNC_WARDER;

    m_stop = true;
    m_synchronizer.Lock();
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    m_synchronizer.Unlock();
    WaitForSingleObject(m_thread, INFINITE);
}

void SocketService::HeartBeat(void *self) {
    ((SocketService *)self)->HeartBeat();
}

void SocketService::HeartBeat() {
    FUNC_WARDER;

    while (!m_stop) {
        // for fast exit and reduce frequently calling time(NULL)
        for (int i = 0; i < 200; i++) {
            Sleep(25ul);
            if (m_stop) {
                break;
            }
        }
        long t = (long)time(NULL);
        if (t - m_last_sent_time > 5) {
            if (SendData(" ", 1) != 1) {
                Init();
            }
        }
    }
}

bool SocketService::Lookup(SOCKADDR_IN *socket_addr) {
    struct addrinfo *answer = NULL, hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    if (0 == getaddrinfo(m_server, NULL, &hint, &answer)) {
        // get the first address
        if (answer->ai_family == AF_INET) {
            memcpy(socket_addr, answer->ai_addr, sizeof(SOCKADDR_IN));
            freeaddrinfo(answer);
            return true;
        }
    }
    return false;
}

void SocketService::Init() {
    FUNC_WARDER;

    if (m_stop) {
        return;
    }

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
        goto error_exit;
    }
    server_addr.sin_port = htons(m_port);

    unsigned long mode;  // for the sake of goto
    mode = 1ul;
    ioctlsocket(m_socket, FIONBIO, &mode);
    if (connect(m_socket, (sockaddr *)&server_addr, sizeof(server_addr)) != SOCKET_ERROR ||
        (WSAGetLastError() != WSAEWOULDBLOCK)) {
        LOG("connect error");
        goto error_exit;
    }

    fd_set writefds, expectfds;
    FD_ZERO(&writefds);
    FD_ZERO(&expectfds);
    FD_SET(m_socket, &writefds);
    FD_SET(m_socket, &expectfds);
    struct timeval timeout;  // for the sake of goto
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int result;  // for the sake of goto
    result = select(0, NULL, &writefds, &expectfds, &timeout);

    if (result == 0) {
        LOG("connection timeout");
        goto error_exit;
    }

    if (result == SOCKET_ERROR) {
        LOG("socket select error");
        goto error_exit;
    }

    if (FD_ISSET(m_socket, &expectfds)) {
        LOG("connection has exception");
        goto error_exit;
    }

    if (FD_ISSET(m_socket, &writefds)) {
        LOG("connection succeeds");
        mode = 0ul;
        ioctlsocket(m_socket, FIONBIO, &mode);
        m_synchronizer.Unlock();
        return;
    }

error_exit:
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    m_synchronizer.Unlock();
}

SocketService::SocketService() : m_socket(INVALID_SOCKET), m_last_sent_time(0), m_stop(false) {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        LOG("WSAStartup fails");
    }
    m_thread = (HANDLE)_beginthread(HeartBeat, 0, this);
}

SocketService::~SocketService() {
    WSACleanup();
}
