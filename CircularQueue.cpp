#include "CircularQueue.h"


// 增加消息.缓冲区满返回false
bool CircularQueue::Push(const char* data, int32_t len)
{
    if (nullptr != data && len > 0 && len <= _capacity)
    {
        auto idle = _sendBufferIdleSize.fetch_sub(len);
        // 检查可用字节,可以写入
        if (idle >= len)
        {
            auto allocStart = _sendBufferAllocCount.fetch_add(len);
            auto allocEnd = allocStart + len;
            auto startIndex = allocStart & _capacityMask;
            auto endIndex = allocEnd & _capacityMask;
            //检查是否需要分段写
            if (startIndex < endIndex)
            {
                memcpy(_buffer + startIndex, data, len);
            }
            else
            {
                auto firstLen = _capacity - startIndex;
                memcpy(_buffer + startIndex, data, firstLen);
                memcpy(_buffer, data + firstLen, len - firstLen);
            }
            //int test = 0;
            while (true)
            {
                auto tmp = allocStart;
                if (_sendBufferWriteCount.compare_exchange_weak(tmp, allocEnd))
                {
                    break;
                }
                //else
                //{
                //    ++test;
                //}
            }
            //if (test > 0)
            //{
            //    printf("wait write %d\r\n", test);
            //}
            return true;
        }
        else
        {
            _sendBufferIdleSize.fetch_add(len);
        }
    }
    return false;
}