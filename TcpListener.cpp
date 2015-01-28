#include "TcpListener.h"
#include "TcpSocket.h"
#include <errno.h>

bool InitSocket()
{
    // Initialize Winsock
    WSADATA wsaData;
    auto r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return r == NO_ERROR;
}

TcpListener::TcpListener(int port) :_port(port)
{
    static bool init = InitSocket();
    if (!init)
    {
        init = InitSocket();
    }
    memset(&_endPointBuf, 0, sizeof(_endPointBuf));
    memset(&_acceptOver, 0, sizeof(_acceptOver));
    _acceptOver.OffsetHigh = static_cast<DWORD>(SokcetAsyncOperation::Accept);

}

TcpListener::~TcpListener()
{
    CloseListen();
}


void TcpListener::CloseListen()
{
    if (_socket != INVALID_SOCKET)
    {
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
        _run = false;
    }
}

int TcpListener::Start()
{
    return Start(100);
}

int TcpListener::Start(int backlog)
{
    int result = InitListenSocket(nullptr, _port, backlog);
    if (NO_ERROR == result)
    {
        if (_async)
        {
            result = AcceptAsync();
            if (result != NO_ERROR)
            {
                CloseListen();
            }
        }
        else
        {
            //同步
            _listenThread = new std::thread([this]()
            {
                this->AcceptSync();
            });
        }
    }
    return result;
}


int TcpListener::InitListenSocket(const char* ipAdd, int port, int backlog)
{
    int result = SOCKET_ERROR;
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    //int listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket != INVALID_SOCKET)
    {
        int reuseaddr_value = 1;
        setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr_value, sizeof(int));

        // 设置IP地址
        unsigned long s_add;
        if (ipAdd == nullptr || strcmp("0.0.0.0", ipAdd) == 0)
        {
            s_add = INADDR_ANY;
        }
        else
        {
            s_add = inet_addr(ipAdd);
        }
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = s_add;
        server_addr.sin_port = htons(static_cast<u_short>(port));

        if (::bind(listenSocket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR ||
            ::listen(listenSocket, backlog) == SOCKET_ERROR)
        {
            result = WSAGetLastError();
            closesocket(listenSocket);
        }
        else
        {
            result = NO_ERROR;
            _socket = listenSocket;
            int namelen = sizeof(_socketAddr);
            getsockname(_socket, (struct sockaddr*)&_socketAddr, &namelen);
        }
    }
    return result;
}

void TcpListener::AcceptSync()
{
    _run = true;
    while (_run)
    {
        int client_fd = SOCKET_ERROR;
        int size = sizeof(sockaddr_in);
        int client = ::accept(_socket, (struct sockaddr*)(_endPointBuf + kAddressLen), &size);
        if (client < 0)
        {
            if (EINTR == WSAGetLastError())
            {
                continue;
            }
        }
        if (SOCKET_ERROR != client)
        {
            OnConnect(client);
        }
    }
    closesocket(_socket);
    _socket = INVALID_SOCKET;
}

int TcpListener::AcceptAsync()
{
    _run = true;
    // Associate the listening socket with the completion port
    if (CreateIoCompletionPort((HANDLE)_socket, _compPort, ((ULONG_PTR)this), 0) == NULL)
    {
        return GetLastError();
    }
    // Load the AcceptEx function into memory using WSAIoctl.
    // The WSAIoctl function is an extension of the ioctlsocket()
    // function that can use overlapped I/O. The function's 3rd
    // through 6th parameters are input and output buffers where
    // we pass the pointer to our AcceptEx function. This is used
    // so that we can call the AcceptEx function directly, rather
    // than refer to the Mswsock.lib library.
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    int result = WSAIoctl(_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx, sizeof(GuidAcceptEx),
        &_funcAcceptEx, sizeof(_funcAcceptEx),
        &_receivedBytes, NULL, NULL);

    if (result == SOCKET_ERROR)
    {
        result = WSAGetLastError();
    }
    else
    {
        result = CreateAccept();
    }
    return result;
}


int TcpListener::CreateAccept()
{
    int result = NO_ERROR;
    // Create an accepting socket
    SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (acceptSocket == INVALID_SOCKET)
    {
        result = WSAGetLastError();
        return result;
    }

    _acceptOver.Offset = acceptSocket;
    auto bRetVal = _funcAcceptEx(_socket, acceptSocket, _endPointBuf,
        0, /// _endPointBuf - (addrLen * 2),
        kAddressLen, kAddressLen,
        &_receivedBytes, &_acceptOver);

    if (bRetVal == FALSE)
    {
        auto error = WSAGetLastError();
        if (ERROR_IO_PENDING != error)
        {
            result = error;
            closesocket(acceptSocket);
        }
    }
    return result;
}

void TcpListener::OnCompletion(LPOVERLAPPED_ENTRY arg)
{
    if (arg->lpCompletionKey == (ULONG_PTR)this)
    {
        SOCKET client = (SOCKET)arg->lpOverlapped->Offset;
        OnConnect(client);
        CreateAccept();
    }
}

void TcpListener::OnConnect(SOCKET client)
{
    int flag = 1;
    ::setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    {
        int sd_size = 32 * 1024;
        int op_size = sizeof(sd_size);
        ::setsockopt(client, SOL_SOCKET, SO_SNDBUF, (const char*)&sd_size, op_size);
    }

    sockaddr_in* socketaddress = (sockaddr_in*)(_endPointBuf + kAddressLen);
    auto channel = new TcpSocket();
    channel->SetCompletionPort(_compPort);
    channel->SetSocket(client);
    channel->SetSocketAddr(socketaddress);

    int port = socketaddress->sin_port;
    port = (port >> 8) + ((port << 8) & 0xff00);

    // Associate the accept socket with the completion port
    HANDLE compPort = CreateIoCompletionPort((HANDLE)client, _compPort, ((ULONG_PTR)channel), 0);
    if (compPort == NULL)
    {
        ::closesocket(client);
        delete channel;
    }
    else
    {
        const char* r = "ok";
        channel->SendAsync(r, strlen(r));
        channel->StartReceive();

    }
}