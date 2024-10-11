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
    size_t                                writePos = 0;
    size_t                                readPos  = 0;
    std::atomic<bool>                     isAvailable{false};

    void write(const std::vector<uint8_t>& data)
    {
        for(auto byte : data)
        {
            size_t nextPos = (writePos + 1) % MIDI_BUFFER_SIZE;
            if(nextPos != readPos)
            {
                buffer[writePos] = byte;
                writePos         = nextPos;
            }
        }
        isAvailable = !data.empty();
    }

    int read(unsigned char* mbuf, int nbytes)
    {
        int bytesCopied = 0;
        while(readPos != writePos && bytesCopied < nbytes)
        {
            *mbuf++ = buffer[readPos];
            readPos = (readPos + 1) % MIDI_BUFFER_SIZE;
            bytesCopied++;
        }
        if(readPos == writePos)
        {
            isAvailable = false;
        }
        return bytesCopied;
    }
};


#endif