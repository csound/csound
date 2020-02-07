/*

 OLABuffer.h
 Framebuffer

 Created by Edward Costello on 10/06/2015.
 Copyright (c) 2015 Edward Costello.

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
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301 USA
 */

#include "csdl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct OLABuffer {

        OPDS h;
        MYFLT *outputArgument;
        MYFLT *inputArgument;
        MYFLT *overlapArgument;
        ARRAYDAT *inputArray;
        AUXCH frameSamplesMemory;
        AUXCH framePointerMemory;
        int32_t frameIndex;
        int32_t overlapSampleIndex;
        int32_t readSampleIndex;
        int32_t framesCount;
        int32_t frameSamplesCount;
        int32_t overlapSamplesCount;
        int32_t ksmps;
        MYFLT **frames;
    } OLABuffer;

    int32_t OLABuffer_initialise(CSOUND *csound, OLABuffer *self);
    int32_t OLABuffer_process(CSOUND *csound, OLABuffer *self);

#ifdef __cplusplus
}
#endif
