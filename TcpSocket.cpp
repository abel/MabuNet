#include "TcpSocket.h"


TcpSocket::TcpSocket() :_id(0)
{
    _sendBuffer1 = new char[_capacity];
    _sendBuffer2 = new char[_capacity];
    mOvlSend.Offset = static_cast<int>(SokcetAsyncOperation::Send);
    mOvlRecv.Offset = static_cast<int>(SokcetAsyncOperation::Receive);
}


TcpSocket::~TcpSocket()
{
    AutoLocker alock(&_lock);
    delete[] _sendBuffer1;
    delete[] _sendBuffer2;
}

// 增加消息.缓冲区满返回-1
int32_t TcpSocket::Push(const char* data, int32_t len)
{
    if (nullptr != data && len > 0)
    {
        AutoLocker alock(&_lock);
        if (_size + len <= _capacity)
        {
            memcpy(_sendBuffer1 + _size, data, len);
            return _size += len;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// 获取所有元素
const char*  TcpSocket::PopAll(int32_t& len)
{
    if (_size > 0)
    {
        AutoLocker alock(&_lock);
        if (_size > 0)
        {
            len = _size;
            _size = 0;
            std::swap(_sendBuffer1, _sendBuffer2);
            return _sendBuffer2;
        }
    }
    len = 0;
    return nullptr;
}

void TcpSocket::OnCompletion(LPOVERLAPPED_ENTRY arg)
{
    SokcetAsyncOperation op = static_cast<SokcetAsyncOperation>(arg->lpOverlapped->Offset);
    switch (op)
    {
        case SokcetAsyncOperation::Accept:
            break;
        case SokcetAsyncOperation::Connect:
            break;
        case SokcetAsyncOperation::Disconnect:
            break;
        case SokcetAsyncOperation::Receive:
            ReadCompleted(arg->dwNumberOfBytesTransferred);
            break;
        case SokcetAsyncOperation::ReceiveFrom:
            break;
        case SokcetAsyncOperation::ReceiveMessageFrom:
            break;
        case SokcetAsyncOperation::Send:
            SendCompleted(arg->dwNumberOfBytesTransferred);
            break;
        case SokcetAsyncOperation::SendPackets:
            break;
        case SokcetAsyncOperation::SendTo:
            break;
        default:
            break;
    }
}


bool TcpSocket::SendAsync(const char* data, int32_t len)
{
        if (Push(data, len) > 0)
        {
            // 尝试0变1,锁定Sender;
            if (_sending.TryLock())
            {
                auto buf = PopAll(_waitSendLen);
                if (buf != nullptr)
                {
                    SendData(buf);
                }
            }
            return true;
        }
        else
        {
            // 发送缓冲区已满
            //Disconnect();
        }
    return false;
}


void __stdcall CallBack(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
    )
{

}

// 此方法线程安全. 同时只会有一个线程调用
void TcpSocket::SendData(const char* data)
{
    WSABUF buf;
    buf.buf = (char*)data;
    buf.len = _waitSendLen;
    //WSASend(_socket, &buf, 1, &_sendBytes, 0, &mOvlSend, CallBack);
    if (!(WSASend(_socket, &buf, 1, &_sendBytes, 0, &mOvlSend, NULL) == 0))
    {

    }
}


void TcpSocket::SendCompleted(size_t bytestransferred)
{
    //if (error)
    //{
    //    return;
    //}
    try
    {
        // 部分发送
        assert(_waitSendLen == bytestransferred);
        //_sendtotal += bytestransferred;
        // 发送完成.
        auto buf = PopAll(_waitSendLen);
        if (buf != nullptr)
        {
            SendData(buf);
        }
        else
        {
            _sending.Unlock();
        }
    }
    catch (...)
    {
        _sending.Unlock();
    }
}



bool TcpSocket::StartReceive()
{
    //if (_open)
    //{
    //    return false;
    //}

    //_open = true;
    //_canReceive = true;
    //_canSend = true;

    //if (_eventCallBack)
    //{
    //    _eventCallBack(this, SessionEvent::kSessionEvent_Connected);
    //}
    AsyncReadSome();
    return true;
}


void TcpSocket::AsyncReadSome()
{
    WSABUF buf;
    buf.len = kBufferSize - _remaining;
    buf.buf = _receiver + _remaining;

    DWORD dwRecv = 0;
    DWORD dwFlag = 0;
    if (WSARecv(_socket, &buf, 1, &dwRecv, &dwFlag, &mOvlRecv, NULL) != 0)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {

        }
    }
    else
    {
        ReadCompleted(dwRecv);
    }
    //_socket->async_read_some(boost::asio::buffer(_receiver + _remaining, kBufferSize - _remaining),
    //    boost::bind(&Session::ReadCompleted, shared_from_this(),
    //    boost::asio::placeholders::error,
    //    boost::asio::placeholders::bytes_transferred
    //    ));
}

void TcpSocket::ReceiveAsync(int32_t len)
{
    // 剩余数据量
    int32_t remaining = 0; // _decoder(_receiver, len);

    SendAsync(_receiver, len);

    // 服务端主动断线
    if (remaining < 0)
    {
        Disconnect();
        return;
    }
    if (remaining > 0 && remaining < kBufferSize)
    {
        // 根据剩余量计算新的偏移
        int32_t remainOffset = len - remaining;
        _remaining = remaining;
        if (remainOffset > 0)
        {
            memcpy(_receiver, _receiver + remainOffset, remaining);
        }
    }
    else
    {
        _remaining = 0;
    }
    AsyncReadSome();
}

void TcpSocket::ReadCompleted(size_t bytestransferred)
{
    if (bytestransferred == 0)
    {
        Disconnect();
        return;
    }
    try
    {
        //_receivetotal += bytestransferred;
        ReceiveAsync(static_cast<int32_t>(bytestransferred + _remaining));
    }
    catch (std::runtime_error e)
    {
        Disconnect();
        //LOGD("read completed catch one exception: " << e.what());
    }
}

void TcpSocket::Disconnect(bool sendOver)
{
    if (_open && _closed.TryLock())
    {
        _open = false;
        _canSend = false;
        _canReceive = false;

        //if (_socket->is_open())
        //{
        //    _socket->close();
        //}

        //if (_eventCallBack)
        //{
        //    _eventCallBack(this, SessionEvent::kSessionEvent_Disconnect);
        //}

        //if (_decoder)
        //{
        //    if (sendOver)
        //    {
        //        _decoder(_receiver, 0);
        //    }
        //    _decoder = nullptr;
        //}
    }
}