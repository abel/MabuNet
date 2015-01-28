#pragma once

#ifndef _SPINLOCK_H
#define _SPINLOCK_H
#include <WinBase.h>

class SpinWait
{
    static const int YIELD_THRESHOLD = 10;
    static const int SLEEP_0_EVERY_HOW_MANY_TIMES = 5;
    static const int SLEEP_1_EVERY_HOW_MANY_TIMES = 20;
private:
    int m_count;

public:
    int Count()
    {
        return m_count;
    }

    bool NextSpinWillYield()
    {
        if (m_count <= 10)
        {
            return false;
        }
        return true;
    }

    void SpinOnce()
    {
        if (NextSpinWillYield())
        {
            //CdsSyncEtwBCLProvider.Log.SpinWait_NextSpinWillYield();
            int num = (m_count >= 10) ? (m_count - 10) : m_count;
            if ((num % 20) == 0x13)
            {
                Sleep(1);
            }
            else if ((num % 5) == 4)
            {
                Sleep(0);
            }
            else
            {
                Yield();
            }
        }
        else
        {
            SpinWait(((int)4) << m_count);
        }
        m_count = (m_count == 0x7fffffff) ? 10 : (m_count + 1);
    }


    void Reset()
    {
        m_count = 0;
    }
}

#endif