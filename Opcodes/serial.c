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

#ifndef NO_SERIAL_OPCODES

#include <stdlib.h>
#include <stdint.h>   /* Standard types */
#include <errno.h>
#include <string.h>   /* String function definitions */

#ifndef WIN32
#include <unistd.h>   /* UNIX standard function definitions */
#include <fcntl.h>    /* File control definitions */
#ifndef __wasm__
#include <termios.h>  /* POSIX terminal control definitions */
#endif
#include <sys/ioctl.h>
#else
#include "winsock2.h"
#endif

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"

/* **************************************************
   As far as I can tell his should work on Windows
   as well using "COM1" etc
   ************************************************** */


/*****************************************************

open a port.  baudRate defaults to 9600
        iPort  serialBegin         SPortName [, baudRate ]

close a port
                        serialEnd           iPort

write byte(s) to the port, at i or k rate
                        serialWrite_i       iPort, iByte
                        serialWrite_i       iPort, kByte
                        serialWrite_i       iPort, Sbytes
                        serialWrite         iPort, iByte
                        serialWrite         iPort, kByte
                        serialWrite         iPort, Sbytes

read the next byte from the input buffer
returned value will be in the range of 0-255
        kByte   serialRead          iPort

print to screen any bytes (up to 32k) in input buffer
note that these bytes will be cleared from the buffer.
use this opcode mainly for debugging messages.
if you want to mix debugging and other communication
messages over the same port, you will need to manually
parse the data with the serialRead opcode.
                        serialPrint                     iPort

clear the input buffer
                        serialFlush         iPort


TODO: (might need some kind of threaded buffer-read?)

 kNum   serialAvailable    iPort
 returns number of bytes available to read

 kByte   serialPeekByte      iPort
 returns the next byte in the input buffer
 does not remove the byte from the buffer

*****************************************************/

#ifdef WIN32
typedef struct SERIAL_GLOBALS_ {
    CSOUND  *csound;
    int32_t     maxind;
    HANDLE  handles[10];
} SERIAL_GLOBALS;

static HANDLE get_port(CSOUND *csound, int32_t port)
{
    HANDLE hport;
    SERIAL_GLOBALS *q;
    q = (SERIAL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "serialGlobals_");
    if (q == NULL) {
      csound->ErrorMsg(csound, Str("No ports available"));
      return NULL;
    }
    hport = (HANDLE)q->handles[port];
    return hport;
}
#endif

typedef struct {
    OPDS  h;
    MYFLT *returnedPort;
    STRINGDAT *portName;
    MYFLT *baudRate;
} SERIALBEGIN;
int32_t serialBegin(CSOUND *csound, SERIALBEGIN *p);

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALEND;
int32_t serialEnd(CSOUND *csound, SERIALEND *p);

typedef struct {
    OPDS  h;
    MYFLT *port, *toWrite;
} SERIALWRITE;
int32_t serialWrite(CSOUND *csound, SERIALWRITE *p);

typedef struct {
    OPDS  h;
    MYFLT *rChar, *port;
} SERIALREAD;
int32_t serialRead(CSOUND *csound, SERIALREAD *p);

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALPRINT;
int32_t serialPrint(CSOUND *csound, SERIALPRINT *p);

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALFLUSH;
int32_t serialFlush(CSOUND *csound, SERIALFLUSH *p);


///-----------TODO
typedef struct {
    OPDS  h;
    MYFLT *retVal, *port;
} SERIALAVAIL;
int32_t serialAvailable(CSOUND *csound, SERIALAVAIL *p);

typedef struct {
    OPDS  h;
    MYFLT *retChar, *port;
} SERIALPEEK;
int32_t serialPeekByte(CSOUND *csound, SERIALPEEK *p);
//------------------

#ifndef WIN32
// takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
// and a baud rate (bps) and connects to that port at that speed and 8N1.
// opens the port in fully raw mode so you can send binary data.
// returns valid fd, or -1 on error
int32_t serialport_init(CSOUND *csound, const char* serialport, int32_t baud)
{
    IGN(csound);
#ifndef __wasm__
    struct termios toptions;
    speed_t brate;
#endif
    int32_t fd;

    //csound = NULL;              /* Not used */
    fprintf(stderr,"init_serialport: opening port %s @ %d bps\n",
            serialport,baud);

    fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
    if (UNLIKELY(fd == -1))  {
      perror("init_serialport: Unable to open port ");
      return -1;
    }

#ifndef __wasm__
    if (UNLIKELY(tcgetattr(fd, &toptions) < 0)) {
      perror("init_serialport: Couldn't get term attributes");
      close(fd);
      return -1;
    }
    switch(baud) {
    default:     brate = B9600;   break;
    case 4800:   brate = B4800;   break;
    case 9600:   brate = B9600;   break;
#ifdef B14400
    case 14400:  brate = B14400;  break;
#endif
    case 19200:  brate = B19200;  break;
#ifdef B28800
    case 28800:  brate = B28800;  break;
#endif
    case 38400:  brate = B38400;  break;
    case 57600:  brate = B57600;  break;
    case 115200: brate = B115200; break;
    }
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    // 8N1
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    // no flow control
    toptions.c_cflag &= ~CRTSCTS;

    toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    toptions.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    toptions.c_cc[VMIN]  = 0;
    toptions.c_cc[VTIME] = 20;

    if (UNLIKELY(tcsetattr(fd, TCSANOW, &toptions) < 0)) {
      close(fd);
      perror("init_serialport: Couldn't set term attributes");
      return -1;
    }
#endif

    return fd;
}
#else

int32_t serialport_init(CSOUND *csound, const char* serialport, int32_t baud)
{
    IGN(csound);
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    int32_t i;
    /* NEED TO CREATE A GLOBAL FOR HANDLE */
    SERIAL_GLOBALS *q;
    q = (SERIAL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "serialGlobals_");
    if (q == NULL) {
      if (UNLIKELY(csound->CreateGlobalVariable(csound, "serialGlobals_",
                                               sizeof(SERIAL_GLOBALS)) != 0)) {
        csound->InitError(csound, Str("serial: failed to allocate globals"));
        return -1;
      }
      q = (SERIAL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "serialGlobals_");
      q->csound = csound;
      q->maxind = 0;
    }
    /* WCHAR wport[256]; */
    /* MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED, serialport, */
    /*                     strlen(serialport)+1,  */
    /*                     (LPCSTR)wport, 256); */
    /* hSerial = CreateFile(serialport, GENERIC_READ | GENERIC_WRITE, 0,  */
    /*                      0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); */
    hSerial = CreateFileA(serialport, GENERIC_READ | GENERIC_WRITE, 0,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
 //Check if the connection was successfull
    if (UNLIKELY(hSerial==INVALID_HANDLE_VALUE)) {
      //If not success full display an Error
      return csound->InitError(csound, Str("%s not available.\n"), serialport);
    }
    memset(&dcbSerialParams, 0, sizeof(dcbSerialParams));
    dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
    switch (baud) {
    case 1200:  dcbSerialParams.BaudRate = CBR_1200; break;
    case 2400:  dcbSerialParams.BaudRate = CBR_2400; break;
    case 4800:  dcbSerialParams.BaudRate = CBR_4800; break;
    default:
    case 9600:  dcbSerialParams.BaudRate = CBR_9600; break;
    case 14400:  dcbSerialParams.BaudRate = CBR_14400; break;
    case 19200:  dcbSerialParams.BaudRate = CBR_19200; break;
    case 38400:  dcbSerialParams.BaudRate = CBR_38400; break;
    case 56000:  dcbSerialParams.BaudRate = CBR_56000; break;
    case 57600:  dcbSerialParams.BaudRate = CBR_57600; break;
    case 115200:  dcbSerialParams.BaudRate = CBR_115200; break;
    case 128000:  dcbSerialParams.BaudRate = CBR_128000; break;
    case 256000:  dcbSerialParams.BaudRate = CBR_256000; break;
    }
    dcbSerialParams.ByteSize=8;
    dcbSerialParams.StopBits=ONESTOPBIT;
    dcbSerialParams.Parity=NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);
    for (i=0; i>q->maxind; i++) {
      if (q->handles[i]==NULL) {
        q->handles[i] = hSerial;
        return i;
      }
    }
    if (UNLIKELY(q->maxind>=10)) {
      csound->InitError(csound, Str("Number of serial handles exhausted"));
      return -1;
    }
    q->handles[q->maxind++] = hSerial;
    return q->maxind-1;
}
/* Also
#define BAUD_075        1
#define BAUD_110        2
#define BAUD_134_5      4
#define BAUD_150        8
#define BAUD_300        16
#define BAUD_600        32
#define BAUD_1200       64
#define BAUD_1800       128
#define BAUD_2400       256
#define BAUD_4800       512
#define BAUD_7200       1024
#define BAUD_9600       2048
#define BAUD_14400      4096
#define BAUD_19200      8192
#define BAUD_38400      16384
#define BAUD_56K        32768
#define BAUD_128K       65536
#define BAUD_115200     131072
#define BAUD_57600      262144
*/
#endif


int32_t serialBegin(CSOUND *csound, SERIALBEGIN *p)
{
    MYFLT xx =
      (MYFLT)serialport_init(csound, (char *)p->portName->data, *p->baudRate);
    *p->returnedPort =xx;
    return(xx<0?NOTOK:OK);
}

int32_t serialEnd(CSOUND *csound, SERIALEND *p)
{
    IGN(csound);
#ifdef WIN32
    SERIAL_GLOBALS *q;
    q = (SERIAL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "serialGlobals_");
    if (UNLIKELY(q = NULL))
      return csound->PerfError(csound, &(p->h), Str("Nothing to close"));
    CloseHandle((HANDLE)q->handles[(int32_t)*p->port]);
    q->handles[(int32_t)*p->port] = NULL;
#else
    close((int32_t)*p->port);
#endif
    return OK;
}

int32_t serialWrite(CSOUND *csound, SERIALWRITE *p)
{
    IGN(csound);
#ifdef WIN32
    HANDLE port = get_port(csound, (int32_t)*p->port);
    if (UNLIKELY(port==NULL)) return NOTOK;
#endif
    {
      unsigned char b = *p->toWrite;
#ifndef WIN32
      if (UNLIKELY(write((int32_t)*p->port, &b, 1)<0))
        return NOTOK;
#else
      int32_t nbytes;
      WriteFile(port, &b, 1, (PDWORD)&nbytes, NULL);
#endif
    }
    return OK;
}

int32_t serialWrite_S(CSOUND *csound, SERIALWRITE *p)
{
     IGN(csound);
#ifdef WIN32
    HANDLE port = get_port(csound, (int32_t)*p->port);
    if (UNLIKELY(port==NULL)) return NOTOK;
#endif
#ifndef WIN32
    if (UNLIKELY(write((int32_t)*p->port,
                       ((STRINGDAT*)p->toWrite)->data,
                       ((STRINGDAT*)p->toWrite)->size))!=
        (int64_t) ((STRINGDAT*)p->toWrite)->size) /* Does Windows write behave correctly? */
        return NOTOK;
#else
      int32_t nbytes;
      WriteFile(port,p->toWrite, strlen((char *)p->toWrite),
                (PDWORD)&nbytes, NULL);
#endif
    return OK;
}


int32_t serialRead(CSOUND *csound, SERIALREAD *p)
{
    IGN(csound);
    unsigned char b = 0;
#ifdef WIN32
    size_t bytes;
    HANDLE port = get_port(csound, (int32_t)*p->port);
    if (UNLIKELY(port==NULL)) return NOTOK;
    ReadFile(port, &b, 1, (PDWORD)&bytes, NULL);
#else
    ssize_t bytes;
    bytes = read((int32_t)*p->port, &b, 1);
#endif
    if (bytes > 0)
      *p->rChar = b;
    else
      *p->rChar = -1;

    return OK;
}

int32_t serialPrint(CSOUND *csound, SERIALPRINT *p)
{
    char str[32769];
#ifdef WIN32
    size_t bytes;
    HANDLE port = get_port(csound, (int32_t)*p->port);
    if (UNLIKELY(port==NULL)) return NOTOK;
    ReadFile(port, str, 32768, (PDWORD)&bytes, NULL);
#else
    ssize_t bytes;
    bytes  = read((int32_t)*p->port, str, 32768);
#endif
    if (bytes > 0) {
      str[bytes] = '\0'; // terminate
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", str);
    }
    return OK;
}

int32_t serialFlush(CSOUND *csound, SERIALFLUSH *p)
{
     IGN(csound);
#if !defined(WIN32) && !defined(__wasm__)
    tcflush(*p->port, TCIFLUSH); // who knows if this works...
#endif
    return OK;
}

int32_t serialAvailable(CSOUND *csound, SERIALAVAIL *p)
{
     IGN(csound);  IGN(p);
    //TODO
    return OK;
}
int32_t serialPeekByte(CSOUND *csound, SERIALPEEK *p)
{
     IGN(csound);  IGN(p);
    //TODO
    return OK;
}

/* ********************************************************************** */
// Support for arduino output via serial line

/* Basic design:  when arduinoStart is called it opens serial line like
   serialBegin and also creates a buffer to store incoming values, and a thead
   listen to the input. We use a 0xf8 byte to synchronise, and each value is
   packed with an index; data only sent if it changes. This can be cancelled
   with arduinoStop.
   The arduino opcode checks that there has been a call to arduinoStart and
   with a simple mutex reads the requested value from the buffer.

   Issue: it assumes that the arduino is already running the correct sketch type.
   Issue:  Can we load the sketch from csound?
*/

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
    int32_t stop, portIndex;
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

/* The port is nonblocking. Check cancellation even while waiting for sync
   or the second byte of a value, and avoid spinning on an idle device. */
static int32_t arduino_get_byte(ARDUINO_GLOBALS *q)
{
    unsigned char b;
    while (!ATOMIC_GET(q->stop)) {
#ifdef WIN32
      DWORD bytes;
      if (!ReadFile(q->port, &b, 1, &bytes, NULL)) return -1;
#else
      ssize_t bytes = read(q->port, &b, 1);
      if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK &&
          errno != EINTR) return -1;
#endif
      if (bytes == 1) return b;
      q->csound->Sleep(1);
    }
    return -1;
}

static uintptr_t arduino_listen(void *p)
{
    ARDUINO_GLOBALS *q = (ARDUINO_GLOBALS*)p;
    CSOUND *csound = q->csound;
    int32_t low, hi;
    while ((low = arduino_get_byte(q)) >= 0 && low != 0xf8) {}
    if (low < 0) goto done;
    while ((low = arduino_get_byte(q)) >= 0) {
      if (low == 0xf8) continue;
      hi = arduino_get_byte(q);
      if (hi < 0) break;
      if (hi == 0xf8) continue;
      int32_t index = (hi >> 3) & 0x1f;
      if (index >= MAXSENSORS) continue;
      csound->LockMutex(q->lock);
      q->values[index] = ((hi & 7) << 7) | (low & 0x7f);
      csound->UnlockMutex(q->lock);
    }
 done:
    ATOMIC_SET(q->stop, 1);
    return 0;
}

static void arduino_close_port(CSOUND *csound, ARDUINO_GLOBALS *q)
{
#ifdef WIN32
    SERIAL_GLOBALS *ports = (SERIAL_GLOBALS*)
      csound->QueryGlobalVariable(csound, "serialGlobals_");
    CloseHandle(q->port);
    if (ports != NULL && q->portIndex >= 0 && q->portIndex < ports->maxind &&
        ports->handles[q->portIndex] == q->port)
      ports->handles[q->portIndex] = NULL;
#else
    IGN(csound);
    close(q->port);
#endif
    q->portIndex = -1;
}

static void arduino_shutdown(CSOUND *csound, ARDUINO_GLOBALS *q)
{
    if (q->thread == NULL) return;
    ATOMIC_SET(q->stop, 1);
    csound->JoinThread(q->thread);
    q->thread = NULL;
    arduino_close_port(csound, q);
}

static int32_t arduino_reset(CSOUND *csound, void *data)
{
    ARDUINO_GLOBALS *q = (ARDUINO_GLOBALS*)data;
    arduino_shutdown(csound, q);
    csound->DestroyMutex(q->lock);
    return OK;
}

int32_t arduino_deinit(CSOUND *csound, ARD_START *p)
{
    if (p->q != NULL && p->generation == p->q->generation)
      arduino_shutdown(csound, p->q);
    p->q = NULL;
    return OK;
}

int32_t arduinoStart(CSOUND* csound, ARD_START* p)
{
    ARDUINO_GLOBALS *q = (ARDUINO_GLOBALS*)
      csound->QueryGlobalVariable(csound, "arduinoGlobals_");
    if (q != NULL && q->thread != NULL)
      return csound->InitError(csound, "%s", Str("arduinoStart already running\n"));
    if (q == NULL) {
      if (csound->CreateGlobalVariable(csound, "arduinoGlobals_", sizeof(*q)) != 0)
        return csound->InitError(csound, "%s", Str("arduino: failed to allocate globals"));
      q = (ARDUINO_GLOBALS*)csound->QueryGlobalVariable(csound, "arduinoGlobals_");
      q->csound = csound;
      q->portIndex = -1;
      q->lock = csound->Create_Mutex(0);
      if (q->lock == NULL) {
        csound->DestroyGlobalVariable(csound, "arduinoGlobals_");
        return csound->InitError(csound, "%s", Str("arduino: failed to create mutex"));
      }
      if (csound->RegisterResetCallback(csound, q, arduino_reset) != OK) {
        csound->DestroyMutex(q->lock);
        csound->DestroyGlobalVariable(csound, "arduinoGlobals_");
        return csound->InitError(csound, "%s", Str("arduino: failed to register cleanup"));
      }
    }
    int32_t port = serialport_init(csound, p->portName->data, *p->baudRate);
    if (port < 0)
      return csound->InitError(csound, "%s", Str("failed to open serial line\n"));
    q->portIndex = port;
#ifdef WIN32
    q->port = get_port(csound, port);
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    if (!SetCommTimeouts(q->port, &timeouts)) {
      arduino_close_port(csound, q);
      return csound->InitError(csound, "%s", Str("arduino: failed to set read timeout"));
    }
#else
    q->port = port;
#endif
    csound->LockMutex(q->lock);
    memset(q->values, 0, sizeof(q->values));
    q->generation++;
    csound->UnlockMutex(q->lock);
    ATOMIC_SET(q->stop, 0);
    q->thread = csound->CreateThread(arduino_listen, q);
    if (q->thread == NULL) {
      ATOMIC_SET(q->stop, 1);
      arduino_close_port(csound, q);
      return csound->InitError(csound, "%s", Str("arduino: failed to create thread"));
    }
    p->q = q;
    p->generation = q->generation;
    *p->returnedPort = port;
    return OK;
}

int32_t arduinoReadSetup(CSOUND* csound, ARD_READ* p)
{
    p->q = (ARDUINO_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "arduinoGlobals_");
    if (p->q == NULL || p->q->thread == NULL || *p->port != p->q->portIndex)
      return csound->InitError(csound, "%s", Str("arduinoStart not running\n"));
    p->generation = p->q->generation;
    p->yt1 = FL(0.0);
    /* Initialise port filter */
    if (*p->ihtim != FL(0.0)) {
      p->c2 = pow(0.5, (double)CS_ONEDKR / *p->ihtim);
      p->c1 = 1.0 - p->c2;
    } else {
      p->c2 = FL(0.0); p->c1 = FL(1.0);
    }
    return OK;
}

int32_t arduinoRead(CSOUND* csound, ARD_READ* p)
{
    ARDUINO_GLOBALS *q = p->q;
    MYFLT val;
    if (!(*p->index >= 0 && *p->index < MAXSENSORS))
      return csound->PerfError(csound, &p->h,
                               "%s", Str("out of range\n"));
    int32_t ind = (int32_t)*p->index;
    csound->LockMutex(q->lock);
    if (p->generation != q->generation || ATOMIC_GET(q->stop)) {
      csound->UnlockMutex(q->lock);
      return csound->PerfError(csound, &p->h, "%s", Str("arduinoStart not running\n"));
    }
    val = (MYFLT)q->values[ind];
    csound->UnlockMutex(q->lock);
    p->yt1 = p->c1 * val + p->c2 * p->yt1;
    *p->val = p->yt1;
    return OK;
}

int32_t arduinoReadFSetup(CSOUND* csound, ARD_READF* p)
{
    p->q = (ARDUINO_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "arduinoGlobals_");
    if (p->q == NULL || p->q->thread == NULL || *p->port != p->q->portIndex)
      return csound->InitError(csound, "%s", Str("arduinoStart not running\n"));
    p->generation = p->q->generation;
    return OK;
}

int32_t arduinoReadF(CSOUND* csound, ARD_READF* p)
{
    ARDUINO_GLOBALS *q = p->q;
    float val;
    if (!(*p->index1 >= 0 && *p->index1 < MAXSENSORS &&
          *p->index2 >= 0 && *p->index2 < MAXSENSORS &&
          *p->index3 >= 0 && *p->index3 < MAXSENSORS))
      return csound->PerfError(csound, &p->h, "%s", Str("out of range\n"));
    int32_t ind1 = (int32_t)*p->index1;
    int32_t ind2 = (int32_t)*p->index2;
    int32_t ind3 = (int32_t)*p->index3;
    uint32_t c1, c2, c3;
    csound->LockMutex(q->lock);
    if (p->generation != q->generation || ATOMIC_GET(q->stop)) {
      csound->UnlockMutex(q->lock);
      return csound->PerfError(csound, &p->h, "%s", Str("arduinoStart not running\n"));
    }
    c1 = q->values[ind1];
    c2 = q->values[ind2];
    c3 = q->values[ind3];
    csound->UnlockMutex(q->lock);
    //printf("ind %d val %d\n", ind, q->values[ind]);
    uint32_t bits = (c3<<22)|(c2<<12)|(c1<<2);
    memcpy(&val, &bits, sizeof(val));
    *p->val = (MYFLT)val;
    return OK;
}

int32_t arduinoStop(CSOUND* csound, ARD_START* p)
{
    ARDUINO_GLOBALS *q =
      (ARDUINO_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                     "arduinoGlobals_");
    if (q==NULL)
      csound->Message(csound, "%s\n", Str("arduino not running"));
    else {
      if (*p->returnedPort != q->portIndex && q->thread != NULL)
        return csound->InitError(csound, "%s", Str("Invalid Arduino port"));
      arduino_shutdown(csound, q);
    }
    return OK;
}

// End of arduino code


#define S(x)    sizeof(x)

static OENTRY serial_localops[] = {
    { (char *)"serialBegin", S(SERIALBEGIN), 0,  (char *)"i", (char *)"So",
      (SUBR)serialBegin, (SUBR)NULL, (SUBR)NULL, NULL, 2 },
    { (char *)"serialEnd", S(SERIALEND), 0, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialEnd, (SUBR)NULL, NULL, 2 },
    { (char *)"serialWrite_i", S(SERIALWRITE), 0,  (char *)"", (char *)"ii",
      (SUBR)serialWrite, (SUBR)NULL, (SUBR)NULL, NULL, 2 },
    { (char *)"serialWrite_i.S", S(SERIALWRITE), 0, (char *)"", (char *)"iS",
      (SUBR)serialWrite_S, (SUBR)NULL, (SUBR)NULL, NULL, 2 },
    { (char *)"serialWrite", S(SERIALWRITE), WR, (char *)"", (char *)"ik",
      (SUBR)NULL, (SUBR)serialWrite, (SUBR)NULL, NULL, 2 },
    { (char *)"serialWrite.S", S(SERIALWRITE), WR, (char *)"", (char *)"iS",
      (SUBR)NULL, (SUBR)serialWrite_S, (SUBR)NULL, NULL, 2 },
    { (char *)"serialRead", S(SERIALREAD), 0, (char *)"k", (char *)"i",
      (SUBR)NULL, (SUBR)serialRead, (SUBR)NULL, NULL, 2 },
    { (char *)"serialPrint", S(SERIALPRINT), WR, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialPrint, (SUBR)NULL, NULL, 2 },
    { (char *)"serialFlush", S(SERIALFLUSH), 0, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialFlush, (SUBR)NULL, NULL, 2 },
    { "arduinoStart", S(ARD_START), 0,  "i", "So", (SUBR)arduinoStart, NULL,
      (SUBR) arduino_deinit, NULL, 2 },
    { "arduinoRead", S(ARD_READ), 0, "k", "iio",
      (SUBR)arduinoReadSetup, (SUBR)arduinoRead, (SUBR)NULL, NULL, 2 },
    { "arduinoReadF", S(ARD_READF), 0, "k", "iiii",
      (SUBR)arduinoReadFSetup, (SUBR)arduinoReadF, (SUBR)NULL, NULL, 2 },
    { "arduinoStop", S(ARD_START), 0,  "", "i", (SUBR)arduinoStop, NULL,
      (SUBR)NULL, NULL, 2 },
    /* aliases */
    { (char *)"serialbegin", S(SERIALBEGIN), 0,  (char *)"i", (char *)"So",
      (SUBR)serialBegin, (SUBR)NULL, (SUBR)NULL   },
    { (char *)"serialend", S(SERIALEND), 0, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialEnd, (SUBR)NULL   },
    { (char *)"serialwritei", S(SERIALWRITE), 0,  (char *)"", (char *)"ii",
      (SUBR)serialWrite, (SUBR)NULL, (SUBR)NULL   },
       { (char *)"serialwritei.S", S(SERIALWRITE), 0, (char *)"", (char *)"iS",
      (SUBR)serialWrite_S, (SUBR)NULL, (SUBR)NULL   },
    { (char *)"serialwrite", S(SERIALWRITE), WR, (char *)"", (char *)"ik",
      (SUBR)NULL, (SUBR)serialWrite, (SUBR)NULL   },
    { (char *)"serialwrite.S", S(SERIALWRITE), WR, (char *)"", (char *)"iS",
      (SUBR)NULL, (SUBR)serialWrite_S, (SUBR)NULL   },
    { (char *)"serialread", S(SERIALREAD), 0, (char *)"k", (char *)"i",
      (SUBR)NULL, (SUBR)serialRead, (SUBR)NULL   },
    { (char *)"serialprint", S(SERIALPRINT), WR, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialPrint, (SUBR)NULL   },
    { (char *)"serialflush", S(SERIALFLUSH), 0, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialFlush, (SUBR)NULL   },
    { "arduinostart", S(ARD_START), 0,  "i", "So", (SUBR)arduinoStart, NULL,
      (SUBR) arduino_deinit},
    { "arduinoread", S(ARD_READ), 0, "k", "iio",
      (SUBR)arduinoReadSetup, (SUBR)arduinoRead  },
    { "arduinoreadf", S(ARD_READF), 0, "k", "iiii",
      (SUBR)arduinoReadFSetup, (SUBR)arduinoReadF  },
    { "arduinostop", S(ARD_START), 0,  "", "i", (SUBR)arduinoStop, NULL  },
/* { (char *)"serialAvailable", S(SERIALAVAIL), 0, (char *)"k", (char *)"i", */
/*   (SUBR)NULL, (SUBR)serialAvailable, (SUBR)NULL   }, */
/* { (char *)"serialPeekByte", S(SERIALPEEK),0,  (char *)"k", (char *)"i", */
/*   (SUBR)NULL, (SUBR)serialPeekByte, (SUBR)NULL   } */
};

LINKAGE_BUILTIN(serial_localops)
#endif // ifndef NO_SERIAL_OPCODES
