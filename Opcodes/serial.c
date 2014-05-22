/*****************************************************
251

                        CSOUND SERIAL PORT OPCODES
                          ma++ ingalls, 2011/9/4
                     modified for WIndows John ffitch
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <stdlib.h>
#include <stdint.h>   /* Standard types */
#include <string.h>   /* String function definitions */

#ifndef WIN32
#include <unistd.h>   /* UNIX standard function definitions */
#include <fcntl.h>    /* File control definitions */
#include <termios.h>  /* POSIX terminal control definitions */
#include <sys/ioctl.h>
#else
#include "windows.h"
#endif

#include "csdl.h"

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
    int     maxind;
    HANDLE  handles[10];
} SERIAL_GLOBALS;

static HANDLE get_port(CSOUND *csound, int port)
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
int serialBegin(CSOUND *csound, SERIALBEGIN *p);

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALEND;
int serialEnd(CSOUND *csound, SERIALEND *p);

typedef struct {
    OPDS  h;
    MYFLT *port, *toWrite;
} SERIALWRITE;
int serialWrite(CSOUND *csound, SERIALWRITE *p);

typedef struct {
    OPDS  h;
    MYFLT *rChar, *port;
} SERIALREAD;
int serialRead(CSOUND *csound, SERIALREAD *p);

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALPRINT;
int serialPrint(CSOUND *csound, SERIALPRINT *p);

typedef struct {
    OPDS  h;
    MYFLT *port;
} SERIALFLUSH;
int serialFlush(CSOUND *csound, SERIALFLUSH *p);


///-----------TODO
typedef struct {
    OPDS  h;
    MYFLT *retVal, *port;
} SERIALAVAIL;
int serialAvailable(CSOUND *csound, SERIALAVAIL *p);

typedef struct {
    OPDS  h;
    MYFLT *retChar, *port;
} SERIALPEEK;
int serialPeekByte(CSOUND *csound, SERIALPEEK *p);
//------------------

#ifndef WIN32
// takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
// and a baud rate (bps) and connects to that port at that speed and 8N1.
// opens the port in fully raw mode so you can send binary data.
// returns valid fd, or -1 on error
int serialport_init(CSOUND *csound, const char* serialport, int baud)
{
    struct termios toptions;
    int fd;
    speed_t brate;

    //csound = NULL;              /* Not used */
    fprintf(stderr,"init_serialport: opening port %s @ %d bps\n",
            serialport,baud);

    fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)  {
      perror("init_serialport: Unable to open port ");
      return -1;
    }

    if (tcgetattr(fd, &toptions) < 0) {
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

    if ( tcsetattr(fd, TCSANOW, &toptions) < 0) {
      close(fd);
      perror("init_serialport: Couldn't set term attributes");
      return -1;
    }

    return fd;
}
#else

int serialport_init(CSOUND *csound, const char* serialport, int baud)
{
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    int i;
    /* NEED TO CREATE A GLOBAL FOR HANDLE */
    SERIAL_GLOBALS *q;
    q = (SERIAL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "serialGlobals_");
    if (q == NULL) {
      if (csound->CreateGlobalVariable(csound, "serialGlobals_",
                                       sizeof(SERIAL_GLOBALS)) != 0){
        csound->ErrorMsg(csound, Str("serial: failed to allocate globals"));
        return 0;
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
    if (hSerial==INVALID_HANDLE_VALUE) {
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
    for(i=0; i>q->maxind; i++) {
      if (q->handles[i]==NULL) {
        q->handles[i] = hSerial;
        return i;
      }
    }
    if (q->maxind>=10)
      return csound->InitError(csound, Str("Number of serial handles exhausted"));
    q->handles[q->maxind++] = hSerial;
    return q->maxind-1;
}
/* Also
#define BAUD_075        1
#define BAUD_110        2
#define BAUD_134_5        4
#define BAUD_150        8
#define BAUD_300        16
#define BAUD_600        32
#define BAUD_1200        64
#define BAUD_1800        128
#define BAUD_2400        256
#define BAUD_4800        512
#define BAUD_7200        1024
#define BAUD_9600        2048
#define BAUD_14400        4096
#define BAUD_19200        8192
#define BAUD_38400        16384
#define BAUD_56K        32768
#define BAUD_128K        65536
#define BAUD_115200        131072
#define BAUD_57600        262144
*/
#endif


int serialBegin(CSOUND *csound, SERIALBEGIN *p)
{
    *p->returnedPort =
      (MYFLT)serialport_init(csound, (char *)p->portName->data, *p->baudRate);
    return OK;
}

int serialEnd(CSOUND *csound, SERIALEND *p)
{
#ifdef WN32
    SERIAL_GLOBALS *q;
    q = (SERIAL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                      "serialGlobals_");
    if (q = NULL)
      return csound->PerfError(csound, Str("Nothing to close"));
    CloseHandle((HANDLE)q->handles[(int)p->port]);
    q->handles[(int)*p->port] = NULL;
#else
    close((int)*p->port);
#endif
    return OK;
}

int serialWrite(CSOUND *csound, SERIALWRITE *p)
{
#ifdef WIN32
    HANDLE port = get_port(csound, (int)*p->port);
    if (port==NULL) return NOTOK;
#endif
    {
      unsigned char b = *p->toWrite;
#ifndef WIN32
      if (UNLIKELY(write((int)*p->port, &b, 1)<0))
        return NOTOK;
#else
      int nbytes;
      WriteFile(port, &b, 1, (PDWORD)&nbytes, NULL);
#endif
    }
    return OK;
}

int serialWrite_S(CSOUND *csound, SERIALWRITE *p)
{
#ifdef WIN32
    HANDLE port = get_port(csound, (int)*p->port);
    if (port==NULL) return NOTOK;
#endif
#ifndef WIN32
    if (UNLIKELY(write((int)*p->port,
                       ((STRINGDAT*)p->toWrite)->data,
                       ((STRINGDAT*)p->toWrite)->size))!=
        ((STRINGDAT*)p->toWrite)->size) /* Does Windows write behave correctly? */
        return NOTOK;
#else
      int nbytes;
      WriteFile(port,p->toWrite, strlen((char *)p->toWrite),
                (PDWORD)&nbytes, NULL);
#endif
    return OK;
}


int serialRead(CSOUND *csound, SERIALREAD *p)
{
    unsigned char b = 0;
    ssize_t bytes;
#ifdef WIN32
    HANDLE port = get_port(csound, (int)*p->port);
    if (port==NULL) return NOTOK;
    ReadFile(port, &b, 1, (PDWORD)&bytes, NULL);
#else
    bytes = read((int)*p->port, &b, 1);
#endif
    if (bytes > 0)
      *p->rChar = b;
    else
      *p->rChar = -1;

    return OK;
}

int serialPrint(CSOUND *csound, SERIALPRINT *p)
{
    char str[32769];
    ssize_t bytes;
#ifdef WIN32
    HANDLE port = get_port(csound, (int)*p->port);
    if (port==NULL) return NOTOK;
    ReadFile(port, str, 32768, (PDWORD)&bytes, NULL);
#else
    bytes  = read((int)*p->port, str, 32768);
#endif
    if (bytes > 0) {
      str[bytes] = '\0'; // terminate
      csound->MessageS(csound, CSOUNDMSG_ORCH, str);
    }
    return OK;
}

int serialFlush(CSOUND *csound, SERIALFLUSH *p)
{
#ifndef WIN32
    tcflush(*p->port, TCIFLUSH); // who knows if this works...
#endif
    return OK;
}

int serialAvailable(CSOUND *csound, SERIALAVAIL *p)
{
    //TODO
    return OK;
}
int serialPeekByte(CSOUND *csound, SERIALPEEK *p)
{
    //TODO
    return OK;
}



#define S(x)    sizeof(x)

static OENTRY serial_localops[] = {
    { (char *)"serialBegin", S(SERIALBEGIN), 0, 1, (char *)"i", (char *)"So",
      (SUBR)serialBegin, (SUBR)NULL, (SUBR)NULL   },
    { (char *)"serialEnd", S(SERIALEND), 0, 2, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialEnd, (SUBR)NULL   },
    { (char *)"serialWrite_i", S(SERIALWRITE), 0, 1, (char *)"", (char *)"ii",
      (SUBR)serialWrite, (SUBR)NULL, (SUBR)NULL   },
       { (char *)"serialWrite_i.S", S(SERIALWRITE), 0, 1, (char *)"", (char *)"iS",
      (SUBR)serialWrite_S, (SUBR)NULL, (SUBR)NULL   },
    { (char *)"serialWrite", S(SERIALWRITE), WR, 2, (char *)"", (char *)"ik",
      (SUBR)NULL, (SUBR)serialWrite, (SUBR)NULL   },
    { (char *)"serialWrite.S", S(SERIALWRITE), WR, 2, (char *)"", (char *)"iS",
      (SUBR)NULL, (SUBR)serialWrite_S, (SUBR)NULL   },
    { (char *)"serialRead", S(SERIALREAD), 0, 2, (char *)"k", (char *)"i",
      (SUBR)NULL, (SUBR)serialRead, (SUBR)NULL   },
    { (char *)"serialPrint", S(SERIALPRINT), WR,2, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialPrint, (SUBR)NULL   },
    { (char *)"serialFlush", S(SERIALFLUSH), 0, 2, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialFlush, (SUBR)NULL   },
/* { (char *)"serialAvailable", S(SERIALAVAIL), 0, 2, (char *)"k", (char *)"i", */
/*   (SUBR)NULL, (SUBR)serialAvailable, (SUBR)NULL   }, */
/* { (char *)"serialPeekByte", S(SERIALPEEK),0,  2, (char *)"k", (char *)"i", */
/*   (SUBR)NULL, (SUBR)serialPeekByte, (SUBR)NULL   } */
};

LINKAGE_BUILTIN(serial_localops)
