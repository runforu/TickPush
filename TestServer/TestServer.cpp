#include <WS2tcpip.h>
#include <iostream>
#include <process.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

void HandleIncomingSocket(void* parg) {
    SOCKET client_socket = (SOCKET)parg;
    if (client_socket == INVALID_SOCKET) {
        std::cout << "accept error !" << std::endl;
        return;
    }
    char recv_data[4096];
    while (true) {
        // receive data
        int ret = recv(client_socket, recv_data, sizeof(recv_data) - 1, 0);
        if (ret > 0) {
            recv_data[ret] = 0x00;
            std::cout << "data :[" << recv_data << "]" << std::endl;
        }
        Sleep(100L);
    }
    closesocket(client_socket);
    _endthread();
}

int main(int argc, char* argv[]) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return 0;
    }

    // create socket
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cout << "socket error !" << std::endl;
        return 0;
    }

    // bind ip and port
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8888);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(server_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) {
        std::cout << "bind error !" << std::endl;
    }

    // start listening
    if (listen(server_socket, 5) == SOCKET_ERROR) {
        std::cout << "listen error !" << std::endl;
        return 0;
    }

    // loop data handling
    while (true) {
        SOCKET client_socket;
        sockaddr_in remote_addr;
        int addr_len = sizeof(remote_addr);

        std::cout << "waiting incoming socket...\n" << std::endl;
        client_socket = accept(server_socket, (SOCKADDR*)&remote_addr, &addr_len);
        char buf[20] = {'\0'};
        inet_ntop(AF_INET, (void*)&remote_addr.sin_addr, buf, 16);
        std::cout << "accept a socket: " << buf << std::endl;
        _beginthread(HandleIncomingSocket, 0, (void*)client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}