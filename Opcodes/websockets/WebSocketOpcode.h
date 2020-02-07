/*

 WebSocketOpcode.h
 WebSockets

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


#include <stdbool.h>
#include "csdl.h"
#include "csound.h"
#ifdef __cplusplus
extern "C"
{
#endif


    typedef struct OpcodeArgument OpcodeArgument;
    typedef struct WebSocket WebSocket;

    typedef struct WebSocketOpcode
    {
        OPDS h;
        MYFLT *arguments[20];
        int32_t inputArgumentCount;
        int32_t outputArgumentCount;
        WebSocket *webSocket;
        OpcodeArgument *inputArguments;
        OpcodeArgument *outputArguments;
        bool isRunning;
        CSOUND *csound;
    } WebSocketOpcode;

    int32_t WebSocketOpcode_initialise(CSOUND *csound, WebSocketOpcode *self);
    int32_t WebSocketOpcode_process(CSOUND *csound, WebSocketOpcode *self);
#ifdef __cplusplus
}
#endif
