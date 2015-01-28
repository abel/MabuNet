
#pragma once
#ifndef YUEDONG_ZZSERVER_LOCKER_H
#define YUEDONG_ZZSERVER_LOCKER_H

#include <atomic>

struct Locker
{
private:
    Locker(const Locker&) = delete;
    Locker& operator=(const Locker&) = delete;

public:
    Locker()
    {
        lock.clear(std::memory_order_relaxed);
    }

    inline void Lock()
    {
#ifdef _DEBUG
        for (uint32_t i = 1; i < UINT32_MAX; ++i)
        {
            if (!lock.test_and_set(std::memory_order_acquire))
            {
                return;
            }
            if ((i & 0xfff) == 0)
            {
                printf("lock:%d\r\n", i);
            }
        }
#else
        while (lock.test_and_set(std::memory_order_acquire)) {}
#endif // _DEBUG
    }

    inline void Unlock()
    {
        lock.clear(std::memory_order_release);
    }

    inline bool TryLock()
    {
        return (!lock.test_and_set(std::memory_order_acquire));
    }

    inline bool TryLock(int t)
    {
        for (int i = 0; i <= t; ++i)
        {
            if (!lock.test_and_set(std::memory_order_acquire))
            {
                return true;
            }
        }
        return false;
    }

private:
    std::atomic_flag lock; // (ATOMIC_FLAG_INIT);
};

// 对Locker的简单包装,保证Lock和Unlock成对调用
class AutoLocker
{
    Locker* _locker;
public:
    inline AutoLocker(Locker* locker) : _locker(locker)
    {
        _locker->Lock();
    }
    inline ~AutoLocker()
    {
        _locker->Unlock();
    }
};

#endif