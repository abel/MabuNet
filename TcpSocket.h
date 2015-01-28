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

    // ��ʼ�첽����
    bool StartReceive();

    bool SendAsync(const char* data, int32_t len);

    inline bool SendAsync(const uint8_t* data, int32_t len)
    {
        return SendAsync((const char*)data, len);
    }

private:
    // �˷����̰߳�ȫ.ͬʱֻ����һ���̵߳���
    void SendData(const char* data);
    void SendCompleted(size_t bytestransferred);


    void AsyncReadSome();

    void ReceiveAsync(int32_t len);

    void ReadCompleted(size_t bytestransferred);
    // �ͻ������ӶϿ�
    void Disconnect(bool sendOver = true);
private:
    Locker _sending;                        // ���ڷ�������
    Locker _closed;                         // ��ִ�йر�

    // ������
    std::function<int32_t(char*, int32_t)> _decoder;

    // �����¼��ص�
    std::function<void(TcpSocket* sender, SokcetAsyncOperation eventcode)> _eventCallBack;

    size_t _sendtotal = 0;                  // �ѷ����ֽ���
    size_t _receivetotal = 0;               // �ѽ����ֽ���

    static const int kBufferSize = 65536;
    const int32_t _id;                      // ����ID,����
    bool _open = false;                     // �ѳ�ʼ��
    bool _canReceive = false;               // �Ƿ���Խ���
    bool _canSend = false;                  // �Ƿ���Է���


    int32_t _remaining = 0;                 // ��ȡ���ʣ������
    char _receiver[kBufferSize];            // ���ջ���

    OVERLAPPED              mOvlRecv;
    OVERLAPPED              mOvlSend;


    // ������Ϣ.������������-1
    int32_t Push(const char* data, int32_t len);
    // ��ȡ����Ԫ��
    const char* PopAll(int32_t& len);

    int32_t _waitSendLen;                   // �����е����ݳ���

    DWORD _sendBytes = 0;
    DWORD _receBytes = 0;
    Locker _lock;
    const int32_t _capacity = 1024;
    int32_t _size = 0;
    // ʹ��˫��Ϣ����
    char* _sendBuffer1;
    char* _sendBuffer2;



};

#endif