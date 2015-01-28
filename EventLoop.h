#pragma once
#ifndef _EVENTLOOP_H
#define _EVENTLOOP_H

#include <winsock2.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include "Locker.h"

enum class SokcetAsyncOperation :int
{
    None = 0,
    Accept = 1,
    Connect,
    Disconnect,
    Receive,
    ReceiveFrom,
    ReceiveMessageFrom,
    Send,
    SendPackets,
    SendTo
};

class SocketCompletionEvent
{
public:
    virtual ~SocketCompletionEvent(){}
    virtual void OnCompletion(LPOVERLAPPED_ENTRY e) = 0;

    void SetCompletionPort(HANDLE compPort)
    {
        _compPort = compPort;
    }
    void SetSocket(SOCKET socket)
    {
        _socket = socket;
    }
    void SetSocketAddr(const struct sockaddr_in* add)
    {
        memcpy(&_socketAddr, add, sizeof(_socketAddr));
    }
protected:
    HANDLE _compPort = INVALID_HANDLE_VALUE;
    SOCKET _socket = INVALID_SOCKET;
    struct sockaddr_in _socketAddr;
};

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    bool Start();
    bool Start(int threadNum);
    void Stop();

    inline HANDLE GetIOHandle()
    {
        return _compPort;
    }

private:
    void StartIoThread();

private:
    bool                            _run = false;
    HANDLE                          _compPort = nullptr;
    std::thread::id                 _curThreadid;
    std::mutex                      _locker;
    std::vector<std::thread*>       _ioThreads;

};

#endif