/*  
    dumpf.c:

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

#include "cs.h"                                      /*  DUMPF.C  */
#include "dumpf.h"
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern int  openout(char *, int);

int kdmpset(KDUMP *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openout(p->STRARG, 1)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else return initerror(Str(X_1009,"need quoted filename"));
    return OK;
}

int kdmp2set(KDUMP2 *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openout(p->STRARG, 1)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else return initerror(Str(X_1009,"need quoted filename"));
    return OK;
}

int kdmp3set(KDUMP3 *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openout(p->STRARG, 1)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else return initerror(Str(X_1009,"need quoted filename"));
    return OK;
}

int kdmp4set(KDUMP4 *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openout(p->STRARG, 1)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else return initerror(Str(X_1009,"need quoted filename"));
    return OK;
}

static MYFLT kval[4];     /* handle up to four ksig arguments */

static void nkdump(int ofd, int format, int nk)
{
    char  outbuf[80];
    int   len = 0;
    MYFLT *kp = kval;

    switch(format) {               /* place formatted kvals into outbuf */
    case 1: {
      char *bp = outbuf;
      len = nk;
      while (nk--)
        *bp++ = (char)(*kp++ / FL(256.0));
      break;
    }
    case 4: {
      short *bp = (short *) outbuf;
      len = nk * sizeof(short);
      while (nk--)
        *bp++ = (short) *kp++;
      break;
    }
    case 5: {
      long *bp = (long *) outbuf;
      len = nk * sizeof(long);
      while (nk--)
        *bp++ = (long) *kp++;
      break;
    }
    case 6: {
      MYFLT *bp = (MYFLT *) outbuf;
      len = nk * sizeof(MYFLT);
      while (nk--)
        *bp++ = *kp++;
      break;
    }
    case 7:
      *outbuf = '\0';
      while (--nk) {
        sprintf(errmsg,"%ld\t", (long) *kp++);
        strcat(outbuf, errmsg);
      }
      sprintf(errmsg,"%ld\n", (long) *kp);
      strcat(outbuf, errmsg);
      len = strlen(outbuf);
      break;
    case 8: *outbuf = '\0';
      while (--nk) {
        sprintf(errmsg,"%6.4f\t", *kp++);
        strcat(outbuf, errmsg);
      }
      sprintf(errmsg,"%6.4f\n", *kp);
      strcat(outbuf, errmsg);
      len = strlen(outbuf);
      break;
    default: die(Str(X_1337,"unknown kdump format"));
    }
    write(ofd, outbuf, len);            /* now write the buffer */
}


int kdump(KDUMP *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig;
      nkdump(p->fdch.fd, p->format, 1);
    }
    return OK;
}

int kdump2(KDUMP2 *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig1;
      kval[1] = *p->ksig2;
      nkdump(p->fdch.fd, p->format, 2);
    }
    return OK;
}

int kdump3(KDUMP3 *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig1;
      kval[1] = *p->ksig2;
      kval[2] = *p->ksig3;
      nkdump(p->fdch.fd, p->format, 3);
    }
    return OK;
}

int kdump4(KDUMP4 *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig1;
      kval[1] = *p->ksig2;
      kval[2] = *p->ksig3;
      kval[3] = *p->ksig4;
      nkdump(p->fdch.fd, p->format, 4);
    }
    return OK;
}


/* ******************************************************************** */
/* ******** READK and friends; new code 1999 Feb 14 by JPff    ******** */
/* ******************************************************************** */
extern int openin(char*);

int krdset(KREAD *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openin(p->STRARG)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else {
      return initerror(Str(X_1009,"need quoted filename"));
    }
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int krd2set(KREAD2 *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openin(p->STRARG)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else {
      return initerror(Str(X_1009,"need quoted filename"));
    }
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int krd3set(KREAD3 *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openin(p->STRARG)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else {
      return initerror(Str(X_1009,"need quoted filename"));
    }
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int krd4set(KREAD4 *p)
{
    if (*p->ifilcod == SSTRCOD) {       /* open in curdir or pathname */
      if ((p->fdch.fd = openin(p->STRARG)) < 0) {
        sprintf(errmsg,Str(X_210,"Cannot open %s"), retfilnam);
        return initerror(errmsg);
      }
      fdrecord(&p->fdch);
      if ((p->format = (int)*p->iformat) < 1 || p->format > 8) {
        return initerror(Str(X_1335,"unknown format request"));
      }
      if (p->format == 2 || p->format == 3) {
        return initerror(Str(X_587,"alaw and ulaw not implemented here"));
      }
      if ((p->timcount = (long)(*p->iprd * ekr)) <= 0)
        p->timcount = 1;
      p->countdown = p->timcount;
    }
    else {
      return initerror(Str(X_1009,"need quoted filename"));
    }
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

static void nkread(int ifd, int format, int nk)
{
    int   len;
    char  inbuf[256];
    MYFLT *kp = kval;

    switch(format) {               /* place formatted kvals into outbuf */
    case 1: {
      unsigned char *bp = (unsigned char*)inbuf;
      len = nk;
      read(ifd, inbuf, len);            /* now read the buffer */
      while (nk--) {
        *kp++ = FL(256.0) * *bp++;
        break;
      }
    }
    case 4: {
      short *bp = (short *) inbuf;
      len = nk * sizeof(short);
      read(ifd, inbuf, len);            /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT) *bp++;
      break;
    }
    case 5: {
      long *bp = (long *) inbuf;
      len = nk * sizeof(long);
      read(ifd, inbuf, len);            /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT) *bp++;
      break;
    }
    case 6: {
      MYFLT *bp = (MYFLT *) inbuf;
      len = nk * sizeof(MYFLT);
      read(ifd, inbuf, len);            /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT) *bp++;
      break;
    }
    case 7:
      while (nk--) {
        char *bp = inbuf;
        do {                  /* Skip whitespace */
          read(ifd, bp, 1);
        } while (isspace(*bp));
        if (*bp=='+' || *bp=='-') bp++;
        do {                  /* Absorb digits */
          read(ifd, bp++, 1);
        } while (isdigit(*(bp-1)));
        lseek(ifd, (off_t)(-1), SEEK_CUR);
#ifndef USE_DOUBLE
        sscanf(inbuf,"%f", kp);
#else
        sscanf(inbuf,"%lf", kp);
#endif
        kp++;
      }
      break;
    case 8:
      while (nk--) {
        char * bp = inbuf;
        do {                  /* Skip whitespace */
          read(ifd, bp, 1);
        } while (isspace(*bp));
        if (*bp=='+' || *bp=='-') bp++;
        do {                  /* Absorb digits and such*/
          read(ifd, bp++, 1);
        } while (!isspace(*(bp-1)));
#ifndef USE_DOUBLE
        sscanf(inbuf,"%f", kp);
#else
        sscanf(inbuf,"%lf", kp);
#endif
        kp++;
      }
      break;
    default: die(Str(X_1337,"unknown kdump format"));
    }
}

int kread(KREAD *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(p->fdch.fd, p->format, 1);
      *p->k1 = kval[0];
    }
    else *p->k1 = p->k[0];
    return OK;
}

int kread2(KREAD2 *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(p->fdch.fd, p->format, 2);
      *p->k1 = kval[0];
      *p->k2 = kval[1];
    }
    else {
      *p->k1 = p->k[0];
      *p->k2 = p->k[1];
    }
    return OK;
}

int kread3(KREAD3 *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(p->fdch.fd, p->format, 2);
      *p->k1 = kval[0];
      *p->k2 = kval[1];
      *p->k3 = kval[2];
    }
    else {
      *p->k1 = p->k[0];
      *p->k2 = p->k[1];
      *p->k3 = p->k[2];
    }
    return OK;
}

int kread4(KREAD4 *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(p->fdch.fd, p->format, 2);
      *p->k1 = kval[0];
      *p->k2 = kval[1];
      *p->k3 = kval[2];
      *p->k4 = kval[3];
    }
    else {
      *p->k1 = p->k[0];
      *p->k2 = p->k[1];
      *p->k3 = p->k[2];
      *p->k4 = p->k[3];
    }
    return OK;
}
