/*  
    mididevice.c:

    Copyright (C) 1995, 2001, 2002 Barry Vercoe, John ffitch, Victor Lazzarini, 
                  J. Mohr, Istvan Varga

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

#include "cs.h"

/* ********************************************************************** */
/* ********************************************************************** */
/* ***     Machine dependent parts of MIDI                            *** */
/* ********************************************************************** */
/* ********************************************************************** */

extern u_char *mbuf, *bufp, *bufend, *endatp;
#define MBUFSIZ   1024

#ifdef WIN32                    /* IV - Nov 10 2002 */
#undef u_char
#undef u_short
#undef u_int
#undef u_long
#include <windows.h>
#include <mmsystem.h>
#define MBUFSIZ_MASK    (MBUFSIZ-1)             /* for wrap-around */
static unsigned char mbuf_tmp[MBUFSIZ];         /* circular buffer */
static int tmpbuf_ndx_r, tmpbuf_ndx_w;
#else
static int  rtfd = 0;        /* init these to stdin */
#endif

#ifdef HAVE_SGTTY_H
# include <sgtty.h>
#elif HAVE_BSD_SGTTY_H
# include <bsd/sgtty.h>
#endif

#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif

#ifdef SGI
/************************************/
/* obsolete SGI code                */
/*                                  */
/*#    include <sys/termio.h>       */
/*#    include <sys/stropts.h>      */
/*#    include <sys/z8530.h>        */
/*     static struct termio tty;    */
/*     static struct strioctl str;  */
/************************************/

/*******************************************/
/* Irix media library MIDI implementation  */
/* (Victor Lazzarini, feb 2001)            */
/*******************************************/

#include <dmedia/midi.h>
#include <poll.h>
#include <stropts.h>

MDport sgiport;
MDevent* mevent;
struct pollfd midipoll;

/* end media library MIDI implementation */

#elif defined HPUX
#    define INBAUD    EXTB
     static struct sgttyb tty;
#elif defined LINUX                              /* J. Mohr  1995 Oct 17 */
#    include <sys/time.h>

/* Include either termios.h or bsd/sgtty.h depending on autoconf tests */
/* We prefer termios as it appears to be the newer interface */
#  ifdef HAVE_TERMIOS_H
#    include <termios.h>
     struct termios tty;
#  else /* HAVE_TERMIOS_H */
     static struct sgttyb tty;
#  endif /* HAVE_TERMIOS_H */
#    include <errno.h>
#    define INBAUD    EXTB
#elif defined sol
#    include <sys/ioctl.h>
     static struct sgttyb tty;
#    define INBAUD    EXTB  /* ioctl baud rate: EXTA = 19200, EXTB = 38400 */
#elif __BEOS__
#ifdef HAVE_OS_H
#    include <OS.h>
#endif
#    include "CsSvInterface.h"
     static const int32 kPortCapacity = 100;
     static port_id gMidiInPort = B_ERROR;
#elif defined(mac_classic)

#elif !defined(DOSGCC) && !defined(__WATCOMC__)&& !defined(LATTICE) && !defined(WIN32) && !defined(SYMANTEC) && !defined(__EMX__)
#    if defined(__MACH__)
#      define USE_OLD_TTY 1
#    endif
#ifdef HAVE_SYS_IOCTL_H
#    include <sys/ioctl.h>
#endif
#    define INBAUD    EXTB  /* ioctl baud rate: EXTA = 19200, EXTB = 38400 */
     static struct sgttyb tty;
#elif defined MACOSX
        static struct sgttyb tty;
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if defined(sun) || defined(sol)
# if defined(sol)
#   include <sys/ttold.h>
# endif
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef TAB2
#undef XTABS
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#undef FLUSHO
#undef PENDIN
#include <sys/termio.h>
#include <sys/stropts.h>
#endif /* ifdef SUN */

#ifdef NeXTi
#include <mach.h>
#include <servers/netname.h>
#include <midi/midi_server.h>
#include <midi/midi_reply_handler.h>
#include <midi/midi_timer.h>
#include <midi/midi_timer_reply_handler.h>
#include <midi/midi_error.h>
#include <midi/midi_timer_error.h>
port_t dev_port,owner_port,timer_port,recv_port,
    recv_reply_port,neg_port;
port_set_name_t port_set;
msg_header_t *in_msg ;
int midiCnt ;

kern_return_t getMidiData
( void *                arg,
  midi_raw_t    midi_raw_data,
  u_int         midi_raw_dataCnt
)
;

midi_reply_t midi_reply = { getMidiData,0,0,0,0,0 };
#define midiError(msg,no) \
     { if (no != KERN_SUCCESS) { midi_error(msg,no) ; longjmp(glob.exitjmp_,1) ;}}
#define midiTimerError(msg,no) \
     { if (no != KERN_SUCCESS) { midi_timer_error(msg,no) ; longjmp(glob.exitjmp_,1) ;}}
#define machError(msg,no) \
     { if (no != KERN_SUCCESS) { mach_error(msg,no) ; longjmp(glob.exitjmp_,1) ;}}

#endif /* NeXTi */

#ifdef WIN32
HMIDIIN     hMidiIn;                            /* IV - Nov 10 2002 */
void CALLBACK win32_midi_in_handler(HMIDIIN, UINT, DWORD, DWORD, DWORD);
#endif

void OpenMIDIDevice(void)
{
#if defined(WIN32)                              /* IV - Nov 10 2002 */
    int     nr_devs, dev_num;
    MIDIINCAPSA caps;

    nr_devs = (int) midiInGetNumDevs();
    if (nr_devs < 1) {
      die(Str(X_359,"No MIDI device available\n"));
    }
    for (dev_num = 0; dev_num < nr_devs; dev_num++) {
      midiInGetDevCapsA((UINT) dev_num, (LPMIDIINCAPSA) &caps,
                        (UINT) sizeof(MIDIINCAPSA));
      if (!strcmp(O.Midiname, caps.szPname)) break;     /* found device */
    }
    if (dev_num >= nr_devs) {
      /* not found, print error message and list of available devices */
      err_printf("Available MIDI input devices:");
      for (dev_num = 0; dev_num < nr_devs; dev_num++) {
        midiInGetDevCapsA((UINT) dev_num, (LPMIDIINCAPSA) &caps,
                          (UINT) sizeof(MIDIINCAPSA));
        err_printf(" \"%s\"", caps.szPname);
      }
      err_printf("\nMIDI in device \"%s\" not found\n", O.Midiname);
      longjmp(cenviron.exitjmp_,1);
    }
    /* reset circular buffer */
    tmpbuf_ndx_r = tmpbuf_ndx_w = 0;
    /* open device */
    if (midiInOpen(&hMidiIn, (UINT) dev_num, (DWORD) win32_midi_in_handler,
                   (DWORD) 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR
        || midiInStart(hMidiIn) != MMSYSERR_NOERROR) {
      die("Error opening MIDI in device");
    }
#else
    if (strcmp(O.Midiname,"stdin") == 0) {
#if defined(mac_classic) || defined(SYMANTEC) || defined(WIN32) || defined(__BEOS__)   /*** double condition for the moment (see above) ***/
      {  die(Str(X_437,"RT Midi_event Console not implemented")); }
# elif defined (DOSGCC) || defined (__WATCOMC__) || defined (LATTICE)
      { if (O.msglevel & WARNMSG)
        printf(Str(X_118,"WARNING: -M stdin: system has no fcntl reading stdin\n")); }
# else
      if (fcntl(rtfd, F_SETFL, fcntl(rtfd, F_GETFL, 0) | O_NDELAY) < 0) {
        perror(Str(X_757,"fcntl"));
        die(Str(X_117,"-M stdin fcntl failed"));
      }
#endif
    }
    else {                   /* open MIDI device, & set nodelay on reads  */
      if ((rtfd = open(O.Midiname, O_RDONLY | O_NDELAY, 0)) < 0)
        dies(Str(X_210,"cannot open %s"), O.Midiname);
#ifndef SYS5
# if defined (DOSGCC) || defined (__WATCOMC__) || defined (LATTICE) || defined(WIN32) || defined(__EMX__)
      { if (O.msglevel & WARNMSG)
        printf(Str(X_827,"WARNING: have not figured out DOS or BCC fcntl yet !!!\n")); }
# else
      if (fcntl(rtfd, F_SETFL, fcntl(rtfd, F_GETFL, 0) | O_NDELAY) < 0)
        dies(Str(X_756,"fcntl failed on %s"), O.Midiname);
# endif
#endif
#ifdef SGI
      /*******************obsolete SGI code**************************************/
      /*       new implementation using the Irix media library

      {
        int arg;
        tty.c_iflag = IGNBRK;
        tty.c_oflag = 0;
        tty.c_cflag = B9600 | CS8 | CREAD | CLOCAL | HUPCL;
        tty.c_lflag = 0;
        tty.c_line = 1;
        tty.c_cc[VINTR] = 0;
        tty.c_cc[VQUIT] = 0;
        tty.c_cc[VERASE] = 0;
        tty.c_cc[VKILL] = 0;
        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 0;
        ioctl(rtfd, TCSETAF, &tty);

        str.ic_cmd = SIOC_RS422;
        str.ic_timout = 0;
        str.ic_len = 4;
        arg = RS422_ON;
        str.ic_dp = (char *)&arg;
        if (ioctl(rtfd, I_STR, &str) < 0) {
          perror(Str(X_202,"cannot ioctl RS422"));
          exit(1);
        }
        str.ic_cmd = SIOC_EXTCLK;
        str.ic_timout = 0;
        str.ic_len = 4;
        arg = EXTCLK_32X;
        str.ic_dp = (char *)&arg;
        if (ioctl(rtfd, I_STR, &str) < 0) {
          perror(Str(X_199,"cannot ioctl EXTCLK"));
          exit(1);
        }
     }
****************************************************************************/
#elif defined sun
      while (ioctl(rtfd, I_POP, 0) == 0)  /* pop the 2 STREAMS modules */
        ;                               /*  betwn user & uart driver */
      gtty(rtfd, &tty);
      tty.sg_ispeed = (char)15;
      tty.sg_ospeed = (char)15;
      tty.sg_flags &= ~(O_TANDEM | O_ECHO | O_CRMOD | O_ANYP);
      tty.sg_flags |= RAW;
      stty(rtfd, &tty);
      ioctl(rtfd, TCFLSH, 0);
      fcntl(rtfd, F_SETFL, O_NDELAY);
#elif defined NeXTi
      {
        kern_return_t r;
        char *midiPort = "midi1" ; /* "midi0" is A */
        close(rtfd) ; /* easier to close it than hassle with idndefs */
        rtfd = 0 ;
            /* look up midi port on localhost */
        r = netname_look_up(name_server_port, "", midiPort, &dev_port);
        mach_error(Str(X_1282,"timer_track: netname_look_up error"), r);
            /* Become owner of the device. */
        r = port_allocate(task_self(), &owner_port);
        machError(Str(X_592,"allocate owner port"), r);
        neg_port = PORT_NULL;
        r = midi_set_owner(dev_port, owner_port, &neg_port);
        midiError(Str(X_617,"become owner"), r);
            /* Get the timer port for the device. */
        r = midi_get_out_timer_port(dev_port, &timer_port);
        midiError(Str(X_1114,"output timer port"), r);
            /*  Get the receive port for the device. */
        r = midi_get_recv(dev_port, owner_port, &recv_port);
        midiError(Str(X_1157,"recv port"), r);
            /* Find out what time it is (and other vital information). */
        r = port_allocate(task_self(), &recv_reply_port);
        machError(Str(X_594,"allocate timer reply port"), r);
            /* Tell it to ignore system messages we are not interested in. */
        r = midi_set_sys_ignores(recv_port,
                                 (MIDI_IGNORE_ACTIVE_SENS
                                  | MIDI_IGNORE_TIMING_CLCK
                                  | MIDI_IGNORE_START
                                  | MIDI_IGNORE_CONTINUE
                                  | MIDI_IGNORE_STOP
                                  | MIDI_IGNORE_SONG_POS_P));
        machError("midi_set_sys_ignores", r);
            /* Set the protocol to indicate our preferences. */
        r = midi_set_proto(recv_port,
                           MIDI_PROTO_RAW,     /* raw, cooked, or packed */
                           FALSE,              /* absolute time codes wanted */
                           MIDI_PROTO_SYNC_SYS,/* use system clock */
                           10,                 /* 10 clocks before data sent */
                           2,                  /* 2 clock timeout between input chars */
                           8192);              /* maximum output queue size  */
        machError("midi_set_proto", r);
            /* Get it to send us received data from now */
        r = midi_get_data(recv_port, recv_reply_port);
        midiTimerError("midi_get_data", r);
            /* Allocate port set.  */
        r = port_set_allocate(task_self(), &port_set);
        machError(Str(X_593,"allocate port set"), r);
            /* Add data receive port to port set. */
        r = port_set_add(task_self(), port_set, recv_reply_port);
        machError(Str(X_584,"add recv_reply_port to set"), r);
            /* Start the timer up.  */
        r = timer_start(timer_port, owner_port);
        midiError(Str(X_1281,"timer start"), r);
            /* allocate the message structure */
        in_msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
      }
#elif defined LINUX
# ifdef HAVE_TERMIOS_H
      if (isatty(rtfd)) {
        if (tcgetattr(rtfd, &tty) < 0) {
          perror(Str(X_1273,"tcgetattr"));
          die(Str(X_331,"MIDI receive: cannot get termios info."));
        }
        cfmakeraw(&tty);
        if (cfsetispeed(&tty, INBAUD) < 0) {
          perror(Str(X_653,"cfsetispeed"));
          die(Str(X_333,"MIDI receive: cannot set input baud rate."));
        }
        if (tcsetattr(rtfd, TCSANOW, &tty) < 0) {
          perror(Str(X_1274,"tcsetattr"));
          die(Str(X_334,"MIDI receive: cannot set termios."));
        }
      }/* HAVE_TERMIOS_H */
# elif defined HAVE_BSD_SGTTY_H
      if (isatty(rtfd)) {
        if (ioctl(rtfd, TIOCGETP, &tty) < 0) {
          perror(Str(X_946,"ioctl"));
          die(Str(X_332,"MIDI receive: cannot get tty settings."));
        }
        tty.sg_ispeed = INBAUD;            /* set baud rate         */
        tty.sg_flags = RAW;                /* and no I/O processing */
        if (ioctl(rtfd, TIOCSETP, &tty) < 0)
          {
            perror(Str(X_946,"ioctl"));
            die(Str(X_335,"MIDI receive: cannot set tty settings"));
          }
      }
# endif /* HAVE_BSD_SGTTY_H */
#elif !defined(__BEOS__) && !defined(mac_classic)
# if !defined(DOSGCC) && !defined(__WATCOMC__) && !defined(LATTICE) && !defined(WIN32) && !defined(__EMX__)
      ioctl(rtfd, TIOCGETP, &tty);           /* for other machines      */
      tty.sg_ispeed = INBAUD;                /*   set baud rate         */
      tty.sg_flags = RAW;                    /*   and no I/O processing */

      ioctl(rtfd, TIOCSETP, &tty);
# endif
#endif
#ifdef SGI
      /***********************************************/
      /* New SGI media library MIDI support VL, 2001 */
      /***********************************************/

      mdInit();
      if (!(sgiport   = mdOpenInPort(O.Midiname))) {
        printf("Can not open port: %s", O.Midiname);
        longjmp(glob.exitjmp_,1);
      }
      else printf("Opening midi port: %s\n", O.Midiname);
      close(rtfd);                  /* close port as in the NeXT implementation */
      rtfd = mdGetFd(sgiport);      /* get the fd of the open port   */
      mevent = mdMalloc((MBUFSIZ/4)*sizeof(MDevent)); /* MDevent alloc */
      midipoll.events = POLLIN;     /* set the poll attrib */
      midipoll.fd = rtfd;
#endif
#if defined(__BEOS__)
      port_id serverPort = find_port(kServerPortName);
      if (serverPort >= B_OK) {
        /* Create a port for transferring MIDI data. */
        gMidiInPort = create_port(kPortCapacity, O.Midiname);
        if (gMidiInPort >= B_OK) {
          struct ServerMidiPort request;
          status_t status;

          /* Send the request to the server. */
          request.mPort = gMidiInPort;
          request.mClientThread = find_thread(0);

          status = write_port(serverPort, kServerNewMidiInPort, &request, sizeof(request));
          if (status >= B_OK) {
            status = write_port(gMidiInPort, 0, O.Midiname, strlen(O.Midiname));
          }
          if (status < B_OK) {
            /* Something went wrong. */
            delete_port(gMidiInPort);
            gMidiInPort = (port_id)status;
          }
        }
      }
      if (gMidiInPort < B_OK) {
        die(Str(X_1548,"Failed to open MIDI input port to Csound Server."));
      }
#endif
        }
#endif          /* WIN32 IV - Nov 10 2002 */
}

/* IV - Nov 10 2002 */

#ifdef WIN32
void CALLBACK win32_midi_in_handler(HMIDIIN hmin, UINT wMsg, DWORD dwInstance,
                                    DWORD dwParam1, DWORD dwParam2)
{
    int     n = (int) (dwParam1 & (DWORD) 0xFF);

    if (wMsg != MIM_DATA || n < 0x80) return;   /* unknown message */
    if (n >= 0xF0) {            /* system messages */
      switch (n) {
      case 0xF8:        /* timing clock */
      case 0xFA:        /* start */
      case 0xFB:        /* continue */
      case 0xFC:        /* stop */
      case 0xFF:        /* system reset */
        mbuf_tmp[tmpbuf_ndx_w++] = (unsigned char) n;
        tmpbuf_ndx_w &= MBUFSIZ_MASK;
      }
      return;           /* anything else is ignored */
    }
    /* channel messages */
    if ((n & 0xE0) != 0xC0) {
      /* 3 bytes */
      mbuf_tmp[tmpbuf_ndx_w++] = (unsigned char) (dwParam1 & (DWORD) 0xFF);
      tmpbuf_ndx_w &= MBUFSIZ_MASK;
      dwParam1 >>= 8;
    }
    /* 2 bytes */
    mbuf_tmp[tmpbuf_ndx_w++] = (unsigned char) (dwParam1 & (DWORD) 0xFF);
    tmpbuf_ndx_w &= MBUFSIZ_MASK;
    dwParam1 >>= 8;
    mbuf_tmp[tmpbuf_ndx_w++] = (unsigned char) (dwParam1 & (DWORD) 0xFF);
    tmpbuf_ndx_w &= MBUFSIZ_MASK;
}
#endif      /* WIN32 */

long GetMIDIData(void)
{
        int  n;
#ifdef WIN32                                    /* IV - Nov 10 2002 */
    n = 0;      /* count the number of bytes received */
    if (tmpbuf_ndx_r == tmpbuf_ndx_w) return 0L;        /* no data */
    bufp = endatp = mbuf;
    do {
      *endatp++ = mbuf_tmp[tmpbuf_ndx_r++]; tmpbuf_ndx_r &= MBUFSIZ_MASK; n++;
    } while (endatp < bufend && tmpbuf_ndx_r != tmpbuf_ndx_w);
    return (long) n;
#else
#ifdef SGI  /* for new SGI media library implementation*/
    int i, j;
#endif
#ifdef LINUX
    /* For select() call, from David Ratajczak */
    fd_set rfds;
    struct timeval tv;
    int retval;
#endif

#ifdef NeXTi
      in_msg->msg_size = MSG_SIZE_MAX;
      in_msg->msg_local_port = port_set;
      msg_receive(in_msg, RCV_TIMEOUT, 0);
      midiCnt = 0 ;
      if (in_msg->msg_local_port == recv_reply_port)
        midi_reply_handler(in_msg,&midi_reply);
      midi_get_data(recv_port, recv_reply_port);
      if (midiCnt > 0) {
        n = midiCnt ;
        bufp = mbuf ;
        endatp = mbuf + n ;
      }
      else return(0) ;
#elif defined LINUX
      /********  NEW STUFF **********/ /* from David Ratajczak */
      /* Use select() to make truly */
      /* non-blocking call to midi  */
      /******************************/

      /* Watch rtfd to see when it has input. */
      FD_ZERO(&rfds);
      FD_SET(rtfd, &rfds);
      /* return immediately */
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      retval = select(rtfd+1, &rfds, NULL, NULL, &tv);
      /* Don't rely on the value of tv now! */

      if (retval) {
        if (retval<0) printf(Str(X_1185,"sensMIDI: retval errno %d"),errno);
        if ((n = read(rtfd, (char *)mbuf, MBUFSIZ)) > 0) {
          bufp = mbuf;
          endatp = mbuf + n;
        }
        else return(0);
      }
      else return(0);
#elif defined(__BEOS__)
      {
        int32 dummy;
        /* Read MIDI data from server, if available. */
        if ((n = read_port_etc(gMidiInPort, &dummy, mbuf, MBUFSIZ, B_TIMEOUT, 0)) > 0) {
          bufp = mbuf;
          endatp = mbuf + n;

        } else {
          return 0;
        }
      }
#elif defined SGI
      /*****************************************/
      /* SGI media library MIDI implementation */
      /* non-blocking calls to mdReceive()     */
      /* using poll()  VL feb 2001             */
      /*****************************************/

      if (poll(&midipoll, 1, 0)) {
        if ((n = mdReceive(sgiport, mevent, MBUFSIZ/4)) > 0 ) {
          /* n is number of MDevents         */
          /*  put the midi message (3 bytes) */
          /*  from mevent  (MDevent->msg)    */
          /*  in the midi buffer             */

          for (i = 0 ;i < n; i++)
            for (j = 0; j < 3; j++) {
              mbuf[j+i] = mevent[i].msg[j];
            }
          bufp = mbuf;
          endatp = mbuf + n*j;

        }
        else return (0);
      }
      else return (0);
#else
      /**
       * Reads from user-defined MIDI input.
       */
      if (csoundIsExternalMidiEnabled(&cenviron)) {
        int n = csoundExternalMidiRead(&cenviron, mbuf, MBUFSIZ);
        if (n == 0) {
          return 0;
        }
        bufp = mbuf;
        endatp = mbuf + n;
      }
      else {
        return 0;
      }
#endif
        return(0);
#endif          /* WIN32 IV - Nov 10 2002 */
}






#ifdef NeXTi
kern_return_t
 getMidiData(void *arg,midi_raw_t midi_raw_data,u_int midi_raw_dataCnt)
{           /* copy midi data into mbuf for comsumption by sensMidi().  */
            /* Also, set variable midiCnt to number of incoming bytes.  */
    int i;
    for (i = 0; i < midi_raw_dataCnt && i <  MBUFSIZ; i++)
      mbuf[i] = (midi_raw_data++)->data;
    midiCnt = i;
}
#endif


void CloseMIDIDevice(void)
{
#ifdef WIN32                            /* IV - Nov 10 2002 */
    if (midiInStop(hMidiIn) != MMSYSERR_NOERROR  ||
        midiInReset(hMidiIn) != MMSYSERR_NOERROR ||
        midiInClose(hMidiIn) != MMSYSERR_NOERROR) {
      die("Error closing MIDI in device");
    }
#else
    extern int close(int);
#  if defined(__BEOS__)
    /* Close the connection to the server. */
    if (gMidiInPort >= B_OK) {
      delete_port(gMidiInPort);
      gMidiInPort = B_ERROR;
    }
#  endif
#ifndef SGI  /******** for SGI using media library ********/
    if (rtfd) close(rtfd);
#else
    mdFree(mevent);
    if (rtfd) mdClosePort(sgiport);
#endif
#endif      /* WIN32 */
}


void MidiOutShortMsg(unsigned char *data)
{
        /* dummy function */
}
