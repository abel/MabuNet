#pragma once

#ifndef _TCPLISTENER_H
#define _TCPLISTENER_H

#include <winsock2.h>
#include <mswsock.h>
#include "EventLoop.h"


class TcpListener :public SocketCompletionEvent
{
public:
    TcpListener(int port);
    ~TcpListener();

    int Start();
    int Start(int backlog);

    void OnCompletion(LPOVERLAPPED_ENTRY) override;

private:

    int AcceptAsync();
    void AcceptSync();
    void OnConnect(SOCKET client);
    int InitListenSocket(const char* ipAdd, int port, int backlog);
    int CreateAccept();
    void CloseListen();
private:
    static const DWORD kAddressLen = (sizeof(sockaddr_in) + 16);

    int _port;
    bool _run = false;
    bool _async = true;

    std::thread* _listenThread = nullptr;

    WSAOVERLAPPED _acceptOver;

    uint8_t _endPointBuf[kAddressLen * 2];

    LPFN_ACCEPTEX _funcAcceptEx = nullptr;
    DWORD _receivedBytes = 0;
};

#endif