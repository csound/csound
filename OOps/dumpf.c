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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include "csoundCore.h"                              /*  DUMPF.C  */
#include "dumpf.h"
#include <ctype.h>
#include <inttypes.h>

static const int32_t dumpf_format_table[9] = {
  0,
  CSFTYPE_INTEGER_BINARY,
  0,
  0,
  CSFTYPE_INTEGER_BINARY,
  CSFTYPE_INTEGER_BINARY,
  CSFTYPE_FLOATS_BINARY,
  CSFTYPE_INTEGER_TEXT,
  CSFTYPE_FLOATS_TEXT,
};

int32_t kdmpset_S(CSOUND *csound, KDUMP *p) {
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    strNcpy(soundoname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;

}


int32_t kdmpset_p(CSOUND *csound, KDUMP *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundoname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundoname, p->ifilcod, "dumpk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}

int32_t kdmp2set_S(CSOUND *csound, KDUMP2 *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    strNcpy(soundoname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}


int32_t kdmp2set_p(CSOUND *csound, KDUMP2 *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundoname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundoname, p->ifilcod, "dumpk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}


int32_t kdmp3set_S(CSOUND *csound, KDUMP3 *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    strNcpy(soundoname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}



int32_t kdmp3set_p(CSOUND *csound, KDUMP3 *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundoname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundoname, p->ifilcod, "dumpk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}

int32_t kdmp4set_S(CSOUND *csound, KDUMP4 *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
   strNcpy(soundoname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (p->fdch.fd == NULL)
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}

int32_t kdmp4set_p(CSOUND *csound, KDUMP4 *p)
{
    /* open in curdir or pathname */
    char soundoname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundoname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundoname, p->ifilcod, "dumpk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundoname,
                                   "wb", "", dumpf_format_table[p->format], 0);
    if (p->fdch.fd == NULL)
      return csound->InitError(csound, Str("Cannot open %s"), soundoname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = p->timcount;
    return OK;
}

static void nkdump(CSOUND *csound, MYFLT *kp, FILE *ofd, int32_t format,
                   int32_t nk, void *p)
{
    char  buf1[256], outbuf[256];
    int32_t   len = 0;

    switch(format) {               /* place formatted kvals into outbuf */
    case 1: {
      int8_t *bp = (int8_t*) outbuf;
      len = nk;
      while (nk--)
        *bp++ = (int8_t) *kp++;
      break;
    }
    case 4: {
      int16_t *bp = (int16_t*) outbuf;
      len = nk * 2;
      while (nk--)
        *bp++ = (int16_t) *kp++;
      break;
    }
    case 5: {
      int32_t *bp = (int32_t*) outbuf;
      len = nk * 4;
      while (nk--)
        *bp++ = (int32_t) *kp++;
      break;
    }
    case 6: {
      float *bp = (float*) outbuf;
      len = nk * sizeof(float);
      while (nk--)
        *bp++ = (float) *kp++;
      break;
    }
    case 7:
      outbuf[0] = '\0';
      while (--nk) {
        snprintf(buf1, 256, "%" PRId64 "\t", (int64_t)*kp++);
        strlcat(outbuf, buf1, 256);
      }
      snprintf(buf1, 256, "%" PRId64 "\n", (int64_t)*kp);
      strlcat(outbuf, buf1, 256);
      len = strlen(outbuf);
      break;
    case 8: *outbuf = '\0';
      while (--nk) {
        CS_SPRINTF(buf1, "%6.4f\t", *kp++);
        strlcat(outbuf, buf1, 256);
      }
      CS_SPRINTF(buf1, "%6.4f\n", *kp);
      strlcat(outbuf, buf1, 256);
      len = strlen(outbuf);
      break;
    default:
      csound->PerfError(csound,&(((KDUMP *)p)->h),
                        Str("unknown kdump format"));
    }
    if (UNLIKELY(fwrite(outbuf, len, 1, ofd)!=1)) { /* now write the buffer */
      csound->PerfError(csound, &(((KDUMP *)p)->h),
                        Str("write failure in dumpk"));
    }
}

int32_t kdump(CSOUND *csound, KDUMP *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig;
      nkdump(csound, kval, p->f, p->format, 1, p);
    }
    return OK;
}

int32_t kdump2(CSOUND *csound, KDUMP2 *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig1;
      kval[1] = *p->ksig2;
      nkdump(csound, kval, p->f, p->format, 2, p);
    }
    return OK;
}

int32_t kdump3(CSOUND *csound, KDUMP3 *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig1;
      kval[1] = *p->ksig2;
      kval[2] = *p->ksig3;
      nkdump(csound, kval, p->f, p->format, 3, p);
    }
    return OK;
}

int32_t kdump4(CSOUND *csound, KDUMP4 *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      kval[0] = *p->ksig1;
      kval[1] = *p->ksig2;
      kval[2] = *p->ksig3;
      kval[3] = *p->ksig4;
      nkdump(csound, kval, p->f, p->format, 4, p);
    }
    return OK;
}

/* ******************************************************************** */
/* ******** READK and friends; new code 1999 Feb 14 by JPff    ******** */
/* ******************************************************************** */

int32_t krdset_p(CSOUND *csound, KREAD *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
     if (csound->ISSTRCOD(*p->ifilcod))
       strNcpy(soundiname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundiname, p->ifilcod, "readk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krdset_S(CSOUND *csound, KREAD *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    strNcpy(soundiname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krd2set_S(CSOUND *csound, KREAD2 *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
   strNcpy(soundiname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krd2set_p(CSOUND *csound, KREAD2 *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundiname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundiname, p->ifilcod, "readk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krd3set_S(CSOUND *csound, KREAD3 *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    strNcpy(soundiname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krd3set_p(CSOUND *csound, KREAD3 *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundiname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundiname, p->ifilcod, "readk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krd4set_S(CSOUND *csound, KREAD4 *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    strNcpy(soundiname,  ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}

int32_t krd4set_p(CSOUND *csound, KREAD4 *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (UNLIKELY((p->format = (int32_t)*p->iformat) < 1 || p->format > 8)) {
      return csound->InitError(csound, Str("unknown format request"));
    }
    if (UNLIKELY(p->format == 2 || p->format == 3)) {
      return csound->InitError(csound,
                               Str("alaw and ulaw not implemented here"));
    }
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundiname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundiname, p->ifilcod, "readk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", dumpf_format_table[p->format], 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->k[0] = p->k[1] = p->k[2] = p->k[3] = FL(0.0);
    return OK;
}


static void nkread(CSOUND *csound, MYFLT *kp, FILE *ifd, int32_t format, int32_t nk)
{
    int32_t   len;
    char  inbuf[256];
    int in_comment = 0;

    switch(format) {               /* place formatted kvals into outbuf */
    case 1: {
      int8_t *bp = (int8_t*)inbuf;
      len = nk;
      if ((unsigned)len != fread(inbuf, 1, len, ifd)) break;        /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT)*bp++;
      break;
    }
    case 4: {
      int16_t *bp = (int16_t*)inbuf;
      len = nk * 2;
      if ((unsigned)len != fread(inbuf, 1, len, ifd)) break;        /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT)*bp++;
      break;
    }
    case 5: {
      int32_t *bp = (int32_t*)inbuf;
      len = nk * 4;
      if ((unsigned)len != fread(inbuf, 1, len, ifd)) break;        /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT)*bp++;
      break;
    }
    case 6: {
      float *bp = (float*)inbuf;
      len = nk * sizeof(float);
      if ((unsigned)len != fread(inbuf, 1, len, ifd)) break;        /* now read the buffer */
      while (nk--)
        *kp++ = (MYFLT)*bp++;
      break;
    }
    case 7:
      while (nk--) {
        char *bp = inbuf;
        int c;
        /* NOTE: could use nextval() in Engine/fgens.c instead */
        do {                    /* Skip whitespace and comments */
          c = getc(ifd);
          switch (c) {
            case EOF: return;
            case '\n': in_comment = 0; break;
            case '#': case ';': case '<': in_comment = 1; break;
            default: break;
          }
          *bp = (char)c;
        } while (isspace(*bp) || in_comment);
        do {                    /* Absorb digits */
          c = getc(ifd);
          if (c == EOF) return;
          if ((unsigned)(bp - inbuf + 1) >= sizeof(inbuf)) return;
          *(++bp) = (char)c;
        } while (isdigit(*bp) ||
                 *bp=='-' || *bp=='+' || *bp=='.' || *bp=='e' ||*bp=='E');
        ungetc(*bp, ifd); //fseek(ifd, -1L, SEEK_CUR);
        *bp = '\0';
#ifndef USE_DOUBLE
        CS_SSCANF(inbuf,"%f", kp);
#else
        CS_SSCANF(inbuf,"%lf", kp);
#endif
        kp++;
      }
      break;
    case 8:
      while (nk--) {
        char *bp = inbuf;
        int c;
        do {                    /* Skip whitespace and comments */
          c = getc(ifd);
          switch (c) {
            case EOF: return;
            case '\n': in_comment = 0; break;
            case '#': case ';': case '<': in_comment = 1; break;
            default: break;
          }
          *bp = (char)c;
        } while (isspace(*bp) || in_comment);
        do {                    /* Absorb digits and such*/
          c = getc(ifd);
          if (c == EOF) return;
          if ((unsigned)(bp - inbuf + 1) >= sizeof(inbuf)) return;
          *(++bp) = (char)c;
        } while (!isspace(*bp));
        (void)ungetc(*bp, ifd); //fseek(ifd, -1L, SEEK_CUR);
        *bp = '\0';
#ifndef USE_DOUBLE
        CS_SSCANF(inbuf,"%f", kp);
#else
        CS_SSCANF(inbuf,"%lf", kp);
#endif
        kp++;
      }
      break;
    default: csound->Warning(csound,Str("unknown kdump format"));
    }
}

int32_t kread(CSOUND *csound, KREAD *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(csound, kval, p->f, p->format, 1);
      *p->k1 = p->k[0] = kval[0];
    }
    else *p->k1 = p->k[0];
    return OK;
}

int32_t kread2(CSOUND *csound, KREAD2 *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(csound, kval, p->f, p->format, 2);
      *p->k1 = p->k[0] = kval[0];
      *p->k2 = p->k[1] = kval[1];
    }
    else {
      *p->k1 = p->k[0];
      *p->k2 = p->k[1];
    }
    return OK;
}

int32_t kread3(CSOUND *csound, KREAD3 *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(csound, kval, p->f, p->format, 3);
      *p->k1 = p->k[0] = kval[0];
      *p->k2 = p->k[1] = kval[1];
      *p->k3 = p->k[2] = kval[2];
    }
    else {
      *p->k1 = p->k[0];
      *p->k2 = p->k[1];
      *p->k3 = p->k[2];
    }
    return OK;
}

int32_t kread4(CSOUND *csound, KREAD4 *p)
{
    MYFLT kval[4];

    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      nkread(csound, kval, p->f, p->format, 4);
      *p->k1 = p->k[0] = kval[0];
      *p->k2 = p->k[1] = kval[1];
      *p->k3 = p->k[2] = kval[2];
      *p->k4 = p->k[3] = kval[3];
    }
    else {
      *p->k1 = p->k[0];
      *p->k2 = p->k[1];
      *p->k3 = p->k[2];
      *p->k4 = p->k[3];
    }
    return OK;
}

#define INITSIZE 1024

int32_t krdsset_S(CSOUND *csound, KREADS *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    strNcpy(soundiname, ((STRINGDAT *)p->ifilcod)->data, 1023);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", 0, 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->lasts = (char*)csound->Calloc(csound, INITSIZE);
    p->lasts[0] = '\0';
     if (p->str->data == NULL) {
       p->str->data = csound->Calloc(csound, INITSIZE);
        p->str->size = INITSIZE;
    }
    return OK;
}


int32_t krdsset_p(CSOUND *csound, KREADS *p)
{
    /* open in curdir or pathname */
    char soundiname[1024];
    if (csound->ISSTRCOD(*p->ifilcod))
      strNcpy(soundiname, get_arg_string(csound, *p->ifilcod), 1023);
    else csound->strarg2name(csound, soundiname, p->ifilcod, "readk.", 0);
    if (p->fdch.fd != NULL)
      csound_fd_close(csound, &(p->fdch));
    p->fdch.fd = csound->FileOpen2(csound, &(p->f), CSFILE_STD, soundiname, "rb",
                                   "SFDIR;SSDIR", 0, 0);
    if (UNLIKELY(p->fdch.fd == NULL))
      return csound->InitError(csound, Str("Cannot open %s"), soundiname);
    fdrecord(csound, &p->fdch);
    if ((p->timcount = (int32_t)(*p->iprd * CS_EKR)) <= 0)
      p->timcount = 1;
    p->countdown = 0;
    p->lasts = (char*)csound->Malloc(csound, INITSIZE);
    p->lasts[0] = '\0';
     if (p->str->data == NULL) {
       p->str->data = csound->Calloc(csound, INITSIZE);
       p->str->size = INITSIZE;
    }
    return OK;
}


int32_t kreads(CSOUND *csound, KREADS *p)
{
    if (--p->countdown <= 0) {
      p->countdown = p->timcount;
      if (UNLIKELY(fgets(p->lasts, INITSIZE-1,  p->f)==NULL)) {
        csound->PerfError(csound, &(p->h), Str("Read failure in readks"));
      }
    }
    strNcpy((char*) p->str->data, p->lasts, INITSIZE);
    return OK;
}
