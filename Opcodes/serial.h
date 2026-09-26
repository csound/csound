/*****************************************************


                        CSOUND SERIAL PORT OPCODES
                          ma++ ingalls, 2011/9/4
                     modified for WIndows John ffitch

                     extenion for ardiuno by John ffitch 2020
 * based on "Arduino-serial"
 * Copyright (c) 2006, Tod E. Kurt, tod@todbot.com
 * http://todbot.com/blog/

    Copyright (C) 2011 matt ingalls
    based on "Arduino-serial", Copyright (c) 2006, Tod E. Kurt, tod@todbot.com
    http://todbot.com/blog/ and licenced LGPL to csound

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef CSOUND_SERIAL_H
#define CSOUND_SERIAL_H

typedef struct {
    OPDS  h;
    MYFLT *returnedPort;
    STRINGDAT *portName;
    MYFLT *baudRate;
} SERIALBEGIN;

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALEND;

typedef struct {
    OPDS  h;
    MYFLT *port, *toWrite;
} SERIALWRITE;

typedef struct {
    OPDS  h;
    MYFLT *rChar, *port;
} SERIALREAD;

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALPRINT;

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALFLUSH;


///-----------TODO
typedef struct {
    OPDS  h;
    MYFLT *retVal, *port;
} SERIALAVAIL;

typedef struct {
    OPDS  h;
    MYFLT *retChar, *port;
} SERIALPEEK;
//------------------

#define MAXSENSORS (30)

typedef struct {
    CSOUND  *csound;
    void *thread;
#ifdef WIN32
    HANDLE port;
#else
    int32_t port;
#endif
    void *lock;
    /* Windows Interlocked operations require a long, including in C++. */
    long stop;
    int32_t portIndex;
    uint64_t generation;
    int32_t values[MAXSENSORS];
} ARDUINO_GLOBALS;

typedef struct {
    OPDS  h;
    MYFLT *returnedPort;
    STRINGDAT *portName;
    MYFLT *baudRate;
    ARDUINO_GLOBALS *q;
    uint64_t generation;
} ARD_START;

typedef struct {
    OPDS  h;
    MYFLT *val;
    MYFLT *port;
    MYFLT *index;
    MYFLT *ihtim;
    ARDUINO_GLOBALS *q;
    MYFLT c1, c2, yt1;
    uint64_t generation;
} ARD_READ;

typedef struct {
    OPDS  h;
    MYFLT *val;
    MYFLT *port;
    MYFLT *index1;
    MYFLT *index2;
    MYFLT *index3;
    ARDUINO_GLOBALS *q;
    uint64_t generation;
} ARD_READF;

#endif
