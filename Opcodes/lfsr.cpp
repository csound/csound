/*
  lsfr.cpp: Linear Feedback Shift Register opcode

  Copyright (C) 2020 Dave Seidel
  This file is part of Csound.

  Based on code by Patrick Dowling in the Ornament & Crime firmware;
  see original copyright notice below this one.

  Original code may be found at:
  https://github.com/mxmxmx/O_C/blob/master/software/o_c_REV/util/util_turing.h

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

// ORIGINAL COPYRIGHT NOTICE
//
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

/**
 * Linear Feedback Shift Register (LFSR) opcode.
 * 
 * Description
 * 
 *      Output is a series of pseudo-random positive integers. This is the technique
 *      used in so-called "Turing machine" synth modules and is usually used to
 *      generate melodic sequences. This implementation is adapted from the firmware
 *      for the Ornament & Crime module, as used in the Quantermain and Meta-Q apps.
 * 
 * Syntax
 *
 *      knum lfsr ilen, iprob [, iseed]
 *
 *      knum = lfsr(ilen, iprob [, iseed])
 *
 * Initialization
 * 
 *      ilen -- length of shift register, valid values are 1-31 (inclusive). The
 *      larger the length, the larger the resulting integers in the output. You
 *      can use this to constrain the output to a suitable range.
 * 
 *      iprob -- probability, valid values 1-255 (inclusive). Controls the spread
 *      of the output; larger values result in a wider spread of values.
 * 
 *      iseed (optional, default -1) -- initial state of the shift register, as a
 *      pattern of bits. The value is treated as an unsigned integer, so the default
 *      of -1 is effectively all bits on (0b11111111...).
 *  
 * Performance
 * 
 *      knum -- Integer output.
 */

#include <time.h>
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

    int32_t init() {
      srand((uint32_t) time(NULL));

        length_ = inargs[0];
        probability_ = inargs[1];
        shift_register_ = in_count() == 3 ? inargs[2] : 0xffffffff;

        return OK;
    }

    int32_t kperf() {
        outargs[0] = (int) _process();
        return OK;
    }
};

#ifdef BUILD_PLUGINS
#include <modload.h>
void csnd::on_load(Csound *csound) {
  csnd::plugin<LFSR>(csound, "lfsr", "k", "iij", csnd::thread::ik);
}
#else 
extern "C" int32_t lfsr_init_modules(CSOUND *csound) {
  csnd::plugin<LFSR>((csnd::Csound *) csound, "lfsr", "k", "iij", csnd::thread::ik);
  return OK;
}

#endif




