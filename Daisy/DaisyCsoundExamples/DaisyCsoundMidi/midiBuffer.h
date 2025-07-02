#ifndef MIDI_BUFFER
#define MIDI_BUFFER

#include <array>
#include <vector>
#include <atomic>
#include <stdlib.h>

constexpr size_t MIDI_BUFFER_SIZE = 64;

struct MidiBuffer
{
    std::array<uint8_t, MIDI_BUFFER_SIZE> buffer;
    std::atomic<size_t>                   writePos{0};
    std::atomic<size_t>                   readPos{0};
    std::atomic<bool>                     isAvailable{false};

    size_t available_space() const
    {
        size_t wp = writePos.load();
        size_t rp = readPos.load();

        if(wp >= rp)
        {
            return MIDI_BUFFER_SIZE - (wp - rp) - 1;
        }
        else
        {
            return rp - wp - 1;
        }
    }

    bool can_fit(size_t messageSize) const
    {
        return available_space() >= messageSize;
    }

    void write(const std::vector<uint8_t>& data)
    {
        if(data.empty())
            return;

        if(!can_fit(data.size()))
        {
            readPos.store(writePos.load());
        }

        for(auto byte : data)
        {
            size_t wp      = writePos.load();
            size_t nextPos = (wp + 1) % MIDI_BUFFER_SIZE;
            if(nextPos != readPos.load())
            {
                buffer[wp] = byte;
                writePos.store(nextPos);
            }
        }
        isAvailable.store(true);
    }

    int read(unsigned char* mbuf, int nbytes)
    {
        int    bytesCopied = 0;
        size_t rp          = readPos.load();
        size_t wp          = writePos.load();

        while(rp != wp && bytesCopied < nbytes)
        {
            *mbuf++ = buffer[rp];
            rp      = (rp + 1) % MIDI_BUFFER_SIZE;
            bytesCopied++;
        }

        readPos.store(rp);

        if(readPos.load() == writePos.load())
        {
            isAvailable.store(false);
        }
        return bytesCopied;
    }
};

#endif