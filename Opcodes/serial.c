#include <stdlib.h> 
#include <stdint.h>   /* Standard types */
#include <string.h>   /* String function definitions */

#ifndef WINDOWS
#include <unistd.h>   /* UNIX standard function definitions */
#include <fcntl.h>    /* File control definitions */
//#include <errno.h>    /* Error number definitions */
#include <termios.h>  /* POSIX terminal control definitions */
#include <sys/ioctl.h>
//#include <getopt.h>
#endif

#include "csdl.h"
//#include "csound.h"

/* **************************************************
   As far as I can tell his should work on Windows
   as well using "COM1" etc
   ************************************************** */


/*****************************************************
 
			CSOUND SERIAL PORT OPCODES
			  ma++ ingalls, 2011/9/4
 
 * based on "Arduino-serial"
 * Copyleft (c) 2006, Tod E. Kurt, tod@todbot.com
 * http://todbot.com/blog/
 

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
	kByte	serialRead          iPort
    
print to screen any bytes (up to 32k) in input buffer
note that these bytes will be cleared from the buffer.
use this opcode mainly for debugging messages.
if you want to mix debugging and other communication 
messages over the same port, you will need to manually
parse the data with the serialRead opcode.
			serialPrint			iPort
 
clear the input buffer
			serialFlush         iPort

 
TODO: (might need some kind of threaded buffer-read?)
 
 kNum   serialAvailable    iPort
 returns number of bytes available to read
 
 kByte   serialPeekByte      iPort
 returns the next byte in the input buffer
 does not remove the byte from the buffer
 
*****************************************************/

typedef struct {
    OPDS  h;
    MYFLT *returnedPort, *portName, *baudRate;
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


#ifndef WINDOWS
// takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
// and a baud rate (bps) and connects to that port at that speed and 8N1.
// opens the port in fully raw mode so you can send binary data.
// returns valid fd, or -1 on error
int serialport_init(const char* serialport, int baud)
{
    struct termios toptions;
    int fd;
    speed_t brate;

    fprintf(stderr,"init_serialport: opening port %s @ %d bps\n",
            serialport,baud);
	
    fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)  {
      perror("init_serialport: Unable to open port ");
      return -1;
    }
    
    if (tcgetattr(fd, &toptions) < 0) {
      perror("init_serialport: Couldn't get term attributes");
      return -1;
    }
    switch(baud) {
    default:     brate = B9600; break;
    case 4800:   brate=B4800;   break;
    case 9600:   brate=B9600;   break;
#ifdef B14400
    case 14400:  brate=B14400;  break;
#endif
    case 19200:  brate=B19200;  break;
#ifdef B28800
    case 28800:  brate=B28800;  break;
#endif
    case 38400:  brate=B38400;  break;
    case 57600:  brate=B57600;  break;
    case 115200: brate=B115200; break;
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
      perror("init_serialport: Couldn't set term attributes");
      return -1;
    }
	
    return fd;
}
#else
#include <windows.h>

int serialport_init(const char* serialport, int baud)
{
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    WCHAR wport[256];
    MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED, serialport, strlen(src)+1, 
                        wport, 256);
    hSerial = CreateFile(wport, GENERIC_READ | GENERIC_WRITE, 0, 
                         0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    dcbSerial.DCBlength=sizeof(dcbSerialParams);
    switch (baud) {
    case 4800:  dcbSerialParams.BaudRate=CBR_4800; break;
    case 9600:
    default:    dcbSerialParams.BaudRate=CBR_9600; break;
    }
    dcbSerialParams.ByteSize=8;
    dcbSerialParams.StopBits=ONESTOPBIT;
    dcbSerialParams.Parity=NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);
    return (int)hSerial;
}
#endif


int serialBegin(CSOUND *csound, SERIALBEGIN *p)
{
    *p->returnedPort = (MYFLT)serialport_init((char *)p->portName, *p->baudRate);
    return OK;
}

int serialEnd(CSOUND *csound, SERIALEND *p)
{
#ifndef WINDOWS
    close(*p->port);
#else
    CloseHandle(*p->port); 
#endif
    return OK;
}

int serialWrite(CSOUND *csound, SERIALWRITE *p)
{
    if (p->XSTRCODE & 2) {
#ifdef nWINDOWS
      write(*p->port, p->toWrite, strlen((char *)p->toWrite));
#else
      int nbytes;
      WriteFile(*p->port,p->toWrite, strlen((char *)p->toWrite), &nbytes, NULL);
#endif
    }
    else {
      unsigned char b = *p->toWrite;
#ifndef WINDOWS
      write(*p->port, &b, 1);
#else
      int nbytes;
      WriteFile(*p->port, &b, 1, &nbytes, NULL);
#endif
    }

    return OK;
}

int serialRead(CSOUND *csound, SERIALREAD *p)
{
    unsigned char b = 0;
    ssize_t bytes;
#ifndef WINDOWS
    bytes = read(*p->port, &b, 1);
#else
    ReadFile(*p->port, &b, 1, &bytes, NULL));
#endif
    if (bytes > 0)
      *p->rChar = b;
    else 
      *p->rChar = -1;
    
    return OK;
}

int serialPrint(CSOUND *csound, SERIALPRINT *p)
{
    char str[32768];
    ssize_t bytes;
#ifndef WINDOWS
    bytes  = read(*p->port, str, 32768);
#else
    ReadFile(*p->port, str, 32768, &bytes, NULL));
#endif
    if (bytes > 0) {
      str[bytes] = 0; // terminate
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", str);
    }
    return OK;
}

int serialFlush(CSOUND *csound, SERIALFLUSH *p)
{
#ifndef WINDOWS
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

static OENTRY localops[] = {
    { (char *)"serialBegin", S(SERIALBEGIN), 1, (char *)"i", (char *)"So",
      (SUBR)serialBegin, (SUBR)NULL, (SUBR)NULL   },
    { (char *)"serialEnd", S(SERIALEND), 2, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialEnd, (SUBR)NULL   },
    { (char *)"serialWrite_i", S(SERIALWRITE), 1, (char *)"", (char *)"iT",
      (SUBR)serialWrite, (SUBR)NULL, (SUBR)NULL   },
    { (char *)"serialWrite", S(SERIALWRITE), 2, (char *)"", (char *)"iU",
      (SUBR)NULL, (SUBR)serialWrite, (SUBR)NULL   },
    { (char *)"serialRead", S(SERIALREAD), 2, (char *)"k", (char *)"i",
      (SUBR)NULL, (SUBR)serialRead, (SUBR)NULL   },
    { (char *)"serialPrint", S(SERIALPRINT), 2, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialPrint, (SUBR)NULL   },
    { (char *)"serialFlush", S(SERIALFLUSH), 2, (char *)"", (char *)"i",
      (SUBR)NULL, (SUBR)serialFlush, (SUBR)NULL   },
    /* { (char *)"serialAvailable", S(SERIALAVAIL), 2, (char *)"k", (char *)"i", */
    /*   (SUBR)NULL, (SUBR)serialAvailable, (SUBR)NULL   }, */
    /* { (char *)"serialPeekByte", S(SERIALPEEK), 2, (char *)"k", (char *)"i", */
    /*   (SUBR)NULL, (SUBR)serialPeekByte, (SUBR)NULL   } */
};

LINKAGE
