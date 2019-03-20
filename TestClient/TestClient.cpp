#define NOT_MT4_PLUGIN
#include <WS2tcpip.h>
#include <iostream>
#include <winsock2.h>
#include "../SocketService.h"

int main(int argc, char* argv[]) {
    char s[100] = "lmoa-main";
    char* context = NULL;
    char* p = strtok_s(s, "|", &context);
    while (p != NULL) {
        std::cout << p << std::endl;
        p = strtok_s(NULL, "|", &context);
    }

    SYSTEMTIME time;
    GetLocalTime(&time);
    std::cout << "milestone 1: " << time.wSecond << "  " << time.wMilliseconds << std::endl;

    SocketService::Instance().OnConfigChanged("127.0.0.1", 8888);
    SocketService::Instance().SendData("Hello Server 1 2 3 4 5 6 7 8 ", 10);

    getchar();
    return 0;
}
