#include <conio.h>

#include "TcpListener.h"
#include "CircularQueue.h"

const int Num2NX = 1024;
CircularQueue* queue;

void Write()
{
    unsigned char bin[Num2NX];
    while (true)
    {
        int count = (rand() % (Num2NX-2)) & 0xffff;
        bin[0] = count & 0xff;
        bin[1] = (count / 256) & 0xff;
        int tmp = count;
        for (int i = 0; i < count; ++i)
        {
            tmp *= 37;
            bin[i + 2] = (tmp & 0xff);
        }
        if (queue->Push((char*)bin, count + 2))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void Read()
{
    unsigned char bin[Num2NX*2];
    int remain = 0;
    uint64_t r = 0;
    while (true)
    {
        int count = 0;
        const char* rebin = queue->Peek(count);
        if (rebin != nullptr)
        {
            assert(remain + count <= Num2NX*2);
            memcpy(bin + remain, rebin, count);
            queue->Pop(count);
            r += count;

            int offset = 0;
            int total = remain + count;
            while (total >= 2)
            {
                int head = bin[offset] + 256 * (bin[offset+1]);
                if (total < head + 2)
                {
                    break;
                }
                //检查内容
                int tmp = head;
                for (int i = 0; i < head; ++i)
                {
                    tmp *= 37;
                    if (bin[i + offset+ 2] != (tmp & 0xff))
                    {
                        assert(false);
                    }
                }
                printf("pass%d\r\n", head);
                offset += (head + 2);
                total -= (head + 2);
            }
            remain = total;
            if (remain > 0)
            {
                memcpy(bin, bin + offset, remain);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        uint64_t w = queue->GetWriteCount();
        assert(r <= w);
    }
}

int main(int agrc, char** agrv)
{
    queue = new CircularQueue(Num2NX);
    std::thread* t1 = new std::thread(Read);

    for (int i=0; i < 8; ++i)
    {
        std::thread* t = new std::thread(Write);
    }

    std::atomic_ullong _sendBufferWritedIndex;
    _sendBufferWritedIndex = 21;
    uint64_t tem = 22;
    bool sd = _sendBufferWritedIndex.compare_exchange_strong(tem, 223);

    EventLoop loop;
    loop.Start();

    TcpListener listen(8080);
    listen.SetCompletionPort(loop.GetIOHandle());
    listen.Start();

    // 按ESC结束
    while (_getch() != 27) {}

    return 0;
}