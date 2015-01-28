#pragma once

#include <stdint.h>
#include <atomic>

template<uint64_t Num>
struct NumIs2N
{
    static const bool value = (Num > 1) && ((Num % 2) == 0) && (NumIs2N<Num / 2>::value);
};

template<>
struct NumIs2N < 1 >
{
    static const bool value = false;
};

template<>
struct NumIs2N < 2 >
{
    static const bool value = true;
};

// �ֽ�����
class CircularQueue
{
private:
    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;

public:
    CircularQueue(int capacity = 1024)
    {
        uint64_t newCapacity = 16;
        while (newCapacity < capacity)
        {
            newCapacity <<= 1;
        }
        _capacity = newCapacity;
        _capacityMask = newCapacity - 1;
        _sendBufferIdleSize = newCapacity;
        _buffer = new char[newCapacity];
        _sendBufferWriteCount = 0;
        _sendBufferAllocCount = 0;
    }

    ~CircularQueue()
    {
        delete[] _buffer;
    }

public:
    inline int32_t size() const
    {
        return _sendBufferWriteCount - _sendBufferReadCount;
    }

    uint64_t GetWriteCount()const
    {
        return _sendBufferWriteCount;
    }

    // ������Ϣ.������������false
    bool Push(const char* data, int32_t len);

    bool Pop(int32_t len)
    {
        if (_sendBufferReadCount + len <= _sendBufferWriteCount)
        {
            _sendBufferReadCount += len;
            _sendBufferIdleSize.fetch_add(len);
            return true;
        }
        return false;
    }

    // ��ȡ����Ԫ��
    const char* Peek(int32_t& len)
    {
        if (_sendBufferReadCount < _sendBufferWriteCount)
        {
            auto canRead = _sendBufferWriteCount - _sendBufferReadCount;
            auto read = (len > 0 && len <= canRead) ? len : canRead;
            auto startIndex = _sendBufferReadCount & _capacityMask;
            auto endIndex = (_sendBufferReadCount + read) & _capacityMask;
            if (startIndex >= endIndex)
            {
                // ֻ���ص�1��
                read = _capacity - startIndex;
            }
            len = read;
            return _buffer + startIndex;
        }
        return nullptr;
    }

private:
    uint64_t _capacity;
    uint64_t _capacityMask;
    char* _buffer;
    uint64_t _sendBufferReadCount = 0;              // 
    std::atomic_ullong _sendBufferWriteCount;       // ��д���ֽ�����
    std::atomic_ullong _sendBufferAllocCount;       // �ѷ����ֽ�����
    std::atomic_llong _sendBufferIdleSize;          // ���п����ֽ���

};

