/*
  lsfr.cpp: Linear Feedback Shift Register opcode

  Copyright (C) 2020 Dave Seidel
  This file is part of Csound.

  Based on code by Patrick Dowling in the Ornament & Crime firmware;
  see original copyright notice below this one.

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

// Copyright (c) 2016 Patrick Dowling
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <plugin.h>

struct LFSR : csnd::Plugin<1, 3> {
    static constexpr char const *otypes = "k";
    static constexpr char const *itypes = "iij";

    uint8_t length_;
    uint8_t probability_;
    uint32_t shift_register_;

    uint32_t _process() {
        uint32_t shift_register = shift_register_;

        // Toggle LSB; there might be better random options
        if (255 == probability_ || static_cast<uint8_t>((rand() % (255 + 1)) < probability_)) {
            shift_register ^= 0x1;
        }

        uint32_t lsb_mask = 0x1 << (length_ - 1);
        if (shift_register & 0x1) {
            shift_register = (shift_register >> 1) | lsb_mask;
        } else {
            shift_register = (shift_register >> 1) & ~lsb_mask;
        }

        // hack... don't turn all zero ...
        if (!shift_register) {
            shift_register |= ((rand() % (0x2 + 1)) << (length_ - 1));
        }

        shift_register_ = shift_register;
        return shift_register & ~(0xffffffff << length_);
    }

    int init() {
        srand(time(NULL));

        length_ = (uint8_t) inargs[0];
        probability_ = (uint8_t) inargs[1];
        shift_register_ = (uint32_t) (in_count() == 3 ? inargs[2] : 0xffffffff);

        // outargs[0] = _process();
        return OK;
    }

    int kperf() {
        outargs[0] = (int) _process();
        return OK;
    }
};

#include <modload.h>
void csnd::on_load(Csound *csound) {
  csnd::plugin<LFSR>(csound, "lfsr", "k", "iij", csnd::thread::ik);
}
