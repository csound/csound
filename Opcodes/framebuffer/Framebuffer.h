/*

 Framebuffer.h
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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 */

#include "csdl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum ArgumentType
    {
        STRING_VAR,
        ARATE_VAR,
        KRATE_VAR,
        IRATE_VAR,
        ARATE_ARRAY,
        KRATE_ARRAY,
        IRATE_ARRAY,
        UNKNOWN
    } ArgumentType;

    typedef struct Framebuffer {

        OPDS h;
        MYFLT *outputArgument;
        MYFLT *inputArgument;
        MYFLT *sizeArgument;
        ArgumentType inputType;
        ArgumentType outputType;
        MYFLT *buffer;
        AUXCH bufferMemory;
        int elementCount;
        int writeIndex;
        int ksmps;
    } Framebuffer;

    int Framebuffer_initialise(CSOUND *csound, Framebuffer *self);
    int Framebuffer_process(CSOUND *csound, Framebuffer *self);

#ifdef __cplusplus
}
#endif
