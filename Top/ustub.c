/*  
    ustub.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

/**
* Dummy variables and stubs to satisfy the compiler and linker
* when building Csound utility programs.
*/

#include "cs.h"
#include <math.h>
#include <stdarg.h>

OPARMS O, O_;                          
ENVIRON cenviron, cenviron_;           

unsigned    outbufsiz;
void        *outbuf;
char       *choutbuf;               /* char  pntr to above  */
short      *shoutbuf;               /* short pntr           */
long       *lloutbuf;               /* long  pntr           */
MYFLT      *floutbuf;               /* float pntr           */
int        outrange = 0;            /* Count samples out of range */

extern unsigned long nframes;
extern FILE* pin;
extern FILE* pout;


int (*rtrecord)(void *, int);
void (*rtplay)(void *, int);
void (*rtclose)(void);
void (*recopen)(int, int, float, int);
void (*playopen)(int, int, float, int);

void (*audtran)(char *, int);
void (*spoutran)(MYFLT *, int);
void (*spinrecv)(void);
void (*nzerotran)(long);

void nullfn(char *, int);
void chartran(MYFLT *, int);
#ifdef never 
void alawtran(MYFLT *, int);
#endif
#ifdef ULAW
void ulawtran(MYFLT *, int); 
#endif
void shortran(MYFLT *, int);
void longtran(MYFLT *, int); 
void floatran(MYFLT *, int);
void bytetran(MYFLT *, int);

int outfd;
int   block = 0;
long  bytes = 0;

void fdrecord(FDCH *fdchp) {}
int initerror(char *s) { return NOTOK;}
int perferror(char *s) { return NOTOK;}

int  Graphable_(void);           /* initialise windows.  Returns 1 if X ok */
void MakeGraph_(WINDAT *, char *);       /* create wdw for a graph */
void MakeXYin_(XYINDAT *, MYFLT, MYFLT);
                                /* create a mouse input window; init scale */
void DrawGraph_(WINDAT *);       /* update graph in existing window */
void ReadXYin_(XYINDAT *);       /* fetch latest value from ms input wdw */
void KillGraph_(WINDAT *);       /* remove a graph window */
void KillXYin_(XYINDAT *);       /* remove a ms input window */
int  ExitGraph_(void); /* print click-Exit message in most recently active window */
void err_printf(char*, ...);

void
nullfn(char *outbuf, int nbytes)
{
    return;
}

void bytetran(MYFLT *buffer, int size) /* after J. Mohr  1995 Oct 17 */
{             /*   sends HI-ORDER 8 bits of shortsamp, converted to unsigned */
    long   longsmp;
    int    n;

    for (n=0; n<size; n++) {
      if ((longsmp = (long)buffer[n]) >= 0) {   /* +ive samp:   */
        if (longsmp > 32767) {          /* out of range?     */
          longsmp = 32767;              /*   clip and report */
          outrange++;
        }
        else {                          /* ditto -ive samp */
          if (longsmp < -32768) {
            longsmp = -32768;
            outrange++;
          }
        }
      }
      choutbuf[n] = (unsigned char)(longsmp >> 8)^0x80;
    }
}

void
shortran(MYFLT *buffer, int size)       /* fix spout vals and put in outbuf */
{                                       /*      write buffer when full      */
    int n;
    long longsmp;

    for (n=0; n<size; n++) {
        if ((longsmp = (long)buffer[n]) >= 0) {               /* +ive samp:   */
            if (longsmp > 32767) {              /* out of range?     */
                longsmp = 32767;        /*   clip and report */
                outrange++;
            }
        }
        else {
            if (longsmp < -32768) {             /* ditto -ive samp */
                longsmp = -32768;
                outrange++;
            }
        }
        shoutbuf[n] = (short)longsmp;
    }
}

void
chartran(MYFLT *buffer, int size) /* same as above, but 8-bit char output */
                                  /*   sends HI-ORDER 8 bits of shortsamp */
{
    int n;
    long longsmp;

    for (n=0; n<size; n++) {
        if ((longsmp = (long)buffer[n]) >= 0) {       /* +ive samp:   */
            if (longsmp > 32767) {              /* out of range?     */
                longsmp = 32767;        /*   clip and report */
                outrange++;
            }
        }
        else {
            if (longsmp < -32768) {             /* ditto -ive samp */
                longsmp = -32768;
                outrange++;
            }
        }
        choutbuf[n] = (char)(longsmp >> 8);
    }
}

#ifdef never
static void
alawtran(MYFLT *buffer, int size)
{
    die(Str(X_590,"alaw not yet implemented"));
}
#endif

#define MUCLIP  32635
#define BIAS    0x84
#define MUZERO  0x02
#define ZEROTRAP

#ifdef ULAW
void
ulawtran(MYFLT *buffer, int size) /* ulaw-encode spout vals & put in outbuf */
                                 /*     write buffer when full      */
{
    int  n;
    long longsmp;
    int  sign;
    extern char    exp_lut[];               /* mulaw encoding table */
    int sample, exponent, mantissa, ulawbyte;

    for (n=0; n<size; n++) {
        if ((longsmp = (long)buffer[n]) < 0) {        /* if sample negative   */
            sign = 0x80;
            longsmp = - longsmp;                /*  make abs, save sign */
        }
        else sign = 0;
        if (longsmp > MUCLIP) {                 /* out of range?     */
            longsmp = MUCLIP;                   /*   clip and report */
            outrange++;
        }
        sample = longsmp + BIAS;
        exponent = exp_lut[( sample >> 8 ) & 0x7F];
        mantissa = ( sample >> (exponent+3) ) & 0x0F;
        ulawbyte = ~ (sign | (exponent << 4) | mantissa );
#ifdef ZEROTRAP
        if (ulawbyte == 0) ulawbyte = MUZERO;    /* optional CCITT trap */
#endif
        choutbuf[n] = ulawbyte;
    }
}
#endif

void
longtran(MYFLT *buffer, int size) /* send long_int spout vals to outbuf */
                                  /*    write buffer when full      */
{
    int n;

    for (n=0; n<size; n++) {
        lloutbuf[n] = (long) buffer[n];
        if (buffer[n] > (float)(0x7fffffff)) {
            lloutbuf[n] = 0x7fffffff;
            outrange++;
        }
        else if (buffer[n] < - (float)(0x7fffffff)) {
            lloutbuf[n] = - 0x7fffffff;
            outrange++;
        }
        else lloutbuf[n] = (long) buffer[n];
    }
}

void
floatran(MYFLT *buffer, int size)
{
    int n;
    for (n=0; n<size; n++) floutbuf[n] = buffer[n];
}

MYFLT ino(MYFLT x)
{
    MYFLT	y, t, e, de, sde, xi;
    int i;

    y = x * FL(0.5);
    t = FL(1.0e-08);
    e = FL(1.0);
    de = FL(1.0);
    for (i = 1; i <= 25; i++) {
      xi = (MYFLT)i;
      de = de * y / xi;
      sde = de * de;
      e += sde;
      if (e * t > sde)
        break;
    }
    return(e);
}

#ifdef WINDOWS
int  Graphable(void)
{
    return 0;
}

void MakeGraph(WINDAT *x, char *y)
{
}

void MakeXYin(XYINDAT *x, MYFLT y, MYFLT z)
{
}

void DrawGraph(WINDAT *x)
{
}

void ReadXYin(XYINDAT *x)
{
}

void KillGraph(WINDAT *x)
{
}

void KillXYin(XYINDAT *x)
{
}

int  ExitGraph(void)
{
    return 0;
}
#endif

int csoundYield(void* csound)
{
    return 1;
}

#ifndef CWIN

void err_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
#endif

void csoundPrintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

FILE *fopen_path(char *name, char *basename, char *env, char *mode)
{
    FILE *ff;
    char *p;
				/* First try to open name given */
    strcpy(name_full, name);
    if ((ff = fopen(name_full, mode))!=NULL) return ff;
				/* if that fails try in base directory */
    strcpy(name_full, basename);
#if defined(__MWERKS) || defined(SYMANTECS)
    p = strrchr(name_full, ':');
#else
    p = strrchr(name_full, '/');
    if (p==NULL) p = strrchr(name_full, '\\');
#endif
    if (p != NULL) {
      strcpy(p+1, name);
      if ((ff = fopen(name_full, mode))!=NULL) return ff;
				/* Of us env argument */
    }
    if ((p = getenv(env)))
#if defined(__MWERKS) || defined(SYMANTECS)
      sprintf(name_full, "%s:%s", p, name);
#else
      sprintf(name_full, "%s/%s", p, name);
#endif
    if ((ff = fopen(name_full, mode))!=NULL) return ff; 
    return NULL;		/* or give up */
}

void synterr(char *s)
{
  printf("error:  %s\n",s);
}

long named_instr_find (char *name)
{
    err_printf("WARNING: named instruments are not supported ");
    err_printf("by stand-alone utilities\n");
    err_printf("assuming insno = -1 for instr %s\n", name);
    return(-1L);
}

float MOD(float a, float bb)
{
    if (bb==0.0f) return 0.0f;
    else {
      float b = (bb<0 ? -bb : bb);
      int d = (int)(a / b);
      a -= d * b;
      while (a>b) a -= b;
      while (-a>b) a += b;
      return a;
    }
}

int writebuffer(MYFLT * obuf, int length)
{
    spoutran(obuf, length);
    audtran(outbuf, O.sfsampsize*length);
    write(outfd, outbuf, O.sfsampsize*length);
    block++;
    bytes += O.sfsampsize*length;
    if (O.rewrt_hdr) {
      rewriteheader(outfd, bytes);
      lseek(outfd, 0L, SEEK_END); /* Place at end again */
    }
    if (O.heartbeat) {
      if (O.heartbeat==1) {
#ifdef SYMANTEC
        nextcurs();
#elif __BEOS__
        putc('.', stderr); fflush(stderr);
#else
        putc("|/-\\"[block&3], stderr); putc(8,stderr);
#endif
      }
      else if (O.heartbeat==2) putc('.', stderr);
      else if (O.heartbeat==3) {
        int n;
        err_printf( "%d%n", block, &n);
        while (n--) putc(8, stderr);
      }
      else putc(7, stderr);
    }
    return length;
}

void beep(void)
{
#ifdef mills_macintosh
    SysBeep(10000);
#else
    printf(Str(X_28,"%c\tbeep!\n"),'\007');
#endif
}


