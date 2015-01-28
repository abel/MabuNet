#include "EventLoop.h"


EventLoop::EventLoop()
{
    _compPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0); 
}


EventLoop::~EventLoop()
{
    Stop();
}

void EventLoop::StartIoThread()
{
    const int eventEntriesNum = 1024;
    OVERLAPPED_ENTRY eventEntries[eventEntriesNum];
    while (_run)
    {
        ULONG numComplete = 0;
        int rc = GetQueuedCompletionStatusEx(_compPort, eventEntries, eventEntriesNum, &numComplete, INFINITE, false);
        if (rc != 0)
        {
            for (ULONG i = 0; i < numComplete; ++i)
            {
                LPOVERLAPPED_ENTRY ioEvent = eventEntries + i;
                SocketCompletionEvent* completionEvent = (SocketCompletionEvent*)(ioEvent->lpCompletionKey);
                if (completionEvent != nullptr)
                {
                    completionEvent->OnCompletion(ioEvent);
                }
            }
        }
        else
        {
           break;
        }
    }
}

bool EventLoop::Start()
{
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    return Start(SystemInfo.dwNumberOfProcessors * 2);
}

bool EventLoop::Start(int threadNum)
{
    if (_run)
    {
        return false;
    }

    if (_compPort == nullptr)
    {
        return false;
    }
   
    _run = true;
    //创建工作线程
    for (int i = 0; i < threadNum; ++i)
    {

        HANDLE ThreadHandle;
        DWORD ThreadID;
        // Create a server worker thread and pass the completion port to the thread.
        auto ptrThread = new std::thread(&EventLoop::StartIoThread, this);
        _ioThreads.push_back(ptrThread);
    }
    return true;
}

void EventLoop::Stop()
{
    if (_run)
    {
        _run = false;
        for (std::thread* t : _ioThreads)
        {
            t->join();
            delete t;
        }
        _ioThreads.clear();
    }
}