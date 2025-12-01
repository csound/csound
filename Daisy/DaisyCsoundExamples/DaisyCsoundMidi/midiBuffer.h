/*
  midiBuffer.h

  Copyright (C) 2025 Aman Jagawni

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
*/
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
