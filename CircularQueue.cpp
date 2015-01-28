#include "CircularQueue.h"


// ������Ϣ.������������false
bool CircularQueue::Push(const char* data, int32_t len)
{
    if (nullptr != data && len > 0 && len <= _capacity)
    {
        auto idle = _sendBufferIdleSize.fetch_sub(len);
        // �������ֽ�,����д��
        if (idle >= len)
        {
            auto allocStart = _sendBufferAllocCount.fetch_add(len);
            auto allocEnd = allocStart + len;
            auto startIndex = allocStart & _capacityMask;
            auto endIndex = allocEnd & _capacityMask;
            //����Ƿ���Ҫ�ֶ�д
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