#pragma once

#ifndef _TCPSOCKET_H
#define _TCPSOCKET_H

#include <atomic>
#include "EventLoop.h"
#include "Locker.h"


class TcpSocket :public SocketCompletionEvent
{
public:
    TcpSocket();
    ~TcpSocket();

public:
    void OnCompletion(LPOVERLAPPED_ENTRY) override;


    void Send();
    void SendAsync();

public:

    // 开始异步接收
    bool StartReceive();

    bool SendAsync(const char* data, int32_t len);

    inline bool SendAsync(const uint8_t* data, int32_t len)
    {
        return SendAsync((const char*)data, len);
    }

private:
    // 此方法线程安全.同时只会有一个线程调用
    void SendData(const char* data);
    void SendCompleted(size_t bytestransferred);


    void AsyncReadSome();

    void ReceiveAsync(int32_t len);

    void ReadCompleted(size_t bytestransferred);
    // 客户端连接断开
    void Disconnect(bool sendOver = true);
private:
    Locker _sending;                        // 正在发送数据
    Locker _closed;                         // 已执行关闭

    // 解码器
    std::function<int32_t(char*, int32_t)> _decoder;

    // 连接事件回调
    std::function<void(TcpSocket* sender, SokcetAsyncOperation eventcode)> _eventCallBack;

    size_t _sendtotal = 0;                  // 已发送字节数
    size_t _receivetotal = 0;               // 已接收字节数

    static const int kBufferSize = 65536;
    const int32_t _id;                      // 连接ID,自增
    bool _open = false;                     // 已初始化
    bool _canReceive = false;               // 是否可以接收
    bool _canSend = false;                  // 是否可以发送


    int32_t _remaining = 0;                 // 读取后的剩余数量
    char _receiver[kBufferSize];            // 接收缓冲

    OVERLAPPED              mOvlRecv;
    OVERLAPPED              mOvlSend;


    // 增加消息.缓冲区满返回-1
    int32_t Push(const char* data, int32_t len);
    // 获取所有元素
    const char* PopAll(int32_t& len);

    int32_t _waitSendLen;                   // 发送中的数据长度

    DWORD _sendBytes = 0;
    DWORD _receBytes = 0;
    Locker _lock;
    const int32_t _capacity = 1024;
    int32_t _size = 0;
    // 使用双消息队列
    char* _sendBuffer1;
    char* _sendBuffer2;



};

#endif