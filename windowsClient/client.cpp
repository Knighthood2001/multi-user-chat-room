#include<WinSock2.h>
#include<WS2tcpip.h>
#include<Windows.h>
#include<stdio.h>
#include <string>
#include <iostream>

#pragma comment(lib,"Ws2_32.lib")
#define BUF_SIZE 1024


//char szMsg[BUF_SIZE];
std::string szMsg;

unsigned SendMsg(void* arg)
{
    SOCKET sock = *((SOCKET*)arg);
    while (1)
    {
        std::getline(std::cin, szMsg);

        if (szMsg == "QUIT()" || szMsg == "quit()") {
            closesocket(sock);
            exit(0);
        }
        size_t len = szMsg.length();
        if (len > INT_MAX) {
            std::cout << "Message is too long" << std::endl;
            continue;
        }

        send(sock, szMsg.c_str(), static_cast<int>(len), 0);
    }
    return 0;
}
unsigned RecvMsg(void* arg)
{
    SOCKET sock = *((SOCKET*)arg);

    char msg[BUF_SIZE];
    while (1)
    {
        int len = recv(sock, msg, sizeof(msg) - 1, 0);
        if (len == -1)
        {
            return -1;
        }
        msg[len] = '\0';
        printf("%s\n", msg);
    }
    return 0;
}


int main()
{
    //��ʼ��socket����
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        return -1;
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        WSACleanup();
        return -1;
    }

    //����socket
    SOCKET hSock;
    hSock = socket(AF_INET, SOCK_STREAM, 0);

    //�󶨶˿�
    SOCKADDR_IN servAdr;
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_port = htons(9999);
    inet_pton(AF_INET, "43.143.168.49", &servAdr.sin_addr);


    //���ӷ�����
    if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
    {
        printf("connect error : %d", GetLastError());
        return -1;
    }
    else
    {
        printf("��ӭ����˽�������ң���������������û���:");
    }

    //ѭ������Ϣ
    HANDLE hSendHand = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendMsg, (void*)&hSock, 0, NULL);

    //ѭ������Ϣ
    HANDLE hRecvHand = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RecvMsg, (void*)&hSock, 0, NULL);


    //�ȴ��߳̽���
    WaitForSingleObject(hSendHand, INFINITE);
    WaitForSingleObject(hRecvHand, INFINITE);


    closesocket(hSock);
    WSACleanup();

}