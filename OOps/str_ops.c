/*
    str_ops.c:

    Copyright (C) 2005, 2006 Istvan Varga
              (C) 2005       Matt J. Ingalls, John ffitch

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

#include "csoundCore.h"
#define CSOUND_STR_OPS_C    1
#include "str_ops.h"
#include <ctype.h>

#define STRSMAX 8

#ifndef HAVE_SNPRINTF
/* add any compiler/system that has snprintf() */
#if defined(HAVE_C99)
#define HAVE_SNPRINTF   1
#endif
#endif

/* strset by John ffitch */

static void str_set(CSOUND *csound, int ndx, const char *s)
{
    if (csound->strsets == NULL) {
      csound->strsmax = STRSMAX;
      csound->strsets = (char **) csound->Calloc(csound, (csound->strsmax + 1)
                                                         * sizeof(char*));
    }
    if (ndx > (int) csound->strsmax) {
      int   i, newmax;
      /* assumes power of two STRSMAX */
      newmax = (ndx | (STRSMAX - 1)) + 1;
      csound->strsets = (char**) csound->ReAlloc(csound, csound->strsets,
                                                 (newmax + 1) * sizeof(char*));
      for (i = (csound->strsmax + 1); i <= newmax; i++)
        csound->strsets[i] = NULL;
      csound->strsmax = newmax;
    }
    if (UNLIKELY(ndx < 0))    /* -ve index */
      csound->Die(csound, Str("illegal strset index"));

    if (csound->strsets[ndx] != NULL) {
      if (strcmp(s, csound->strsets[ndx]) == 0)
        return;
      if (csound->oparms->msglevel & WARNMSG) {
        csound->Warning(csound, Str("strset index conflict at %d"), ndx);
        csound->Warning(csound, Str("previous value: '%s', replaced with '%s'"),
                                csound->strsets[ndx], s);
      }
      csound->Free(csound, csound->strsets[ndx]);
    }
    csound->strsets[ndx] = (char*) csound->Malloc(csound, strlen(s) + 1);
    strcpy(csound->strsets[ndx], s);
    if ((csound->oparms->msglevel & 7) == 7)
      csound->Message(csound, "Strsets[%d]: '%s'\n", ndx, s);
}

int strset_init(CSOUND *csound, STRSET_OP *p)
{
    str_set(csound, (int) MYFLT2LRND(*p->indx), (char*) p->str);
    return OK;
}

/* for argdecode.c */

void strset_option(CSOUND *csound, char *s)
{
    int indx = 0;

    if (UNLIKELY(!isdigit(*s)))
      csound->Die(csound, Str("--strset: invalid format"));
    do {
      indx = (indx * 10) + (int) (*s++ - '0');
    } while (isdigit(*s));
    if (UNLIKELY(*s++ != '='))
      csound->Die(csound, Str("--strset: invalid format"));
    str_set(csound, indx, s);
}

int strget_init(CSOUND *csound, STRGET_OP *p)
{
    int   indx;

    ((char*) p->r)[0] = '\0';
    if (*(p->indx) == SSTRCOD) {
      if (csound->currevent->strarg == NULL)
        return OK;
      if ((int) strlen(csound->currevent->strarg) >= csound->strVarMaxLen)
        return csound->InitError(csound, Str("strget: buffer overflow"));
      strcpy((char*) p->r, csound->currevent->strarg);
      return OK;
    }
    indx = (int)((double)*(p->indx) + (*(p->indx) >= FL(0.0) ? 0.5 : -0.5));
    if (indx < 0 || indx > (int) csound->strsmax ||
        csound->strsets == NULL || csound->strsets[indx] == NULL)
      return OK;
    if (UNLIKELY((int) strlen(csound->strsets[indx]) >= csound->strVarMaxLen))
      return csound->InitError(csound, Str("strget: buffer overflow"));
    strcpy((char*) p->r, csound->strsets[indx]);
    return OK;
}

static CS_NOINLINE int StrOp_ErrMsg(void *p, const char *msg)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *opname = csound->GetOpcodeName(p);

    if (UNLIKELY(csound->ids != NULL && csound->ids->insdshead == csound->curip))
      return csound->InitError(csound, "%s: %s", opname, Str(msg));
    else if (UNLIKELY(csound->pds != NULL))
      return csound->PerfError(csound, "%s: %s", opname, Str(msg));
    else
      csound->Die(csound, "%s: %s", opname, Str(msg));

    return NOTOK;
}

static CS_NOINLINE CS_NORETURN void StrOp_FatalError(void *p, const char *msg)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *opname = csound->GetOpcodeName(p);

    csound->Die(csound, "%s: %s", opname, Str(msg));
}

/* strcpy */

int strcpy_opcode(CSOUND *csound, STRCPY_OP *p)
{
    char  *newVal = (char*) p->str;

    if (p->r == p->str)
      return OK;
    if (*p->str == SSTRCOD){
       csound->strarg2name(csound, (char *)p->r, p->str, "soundin.", p->XSTRCODE);
       return OK;
    }
    if (UNLIKELY((int) strlen(newVal) >= csound->strVarMaxLen))
      return StrOp_ErrMsg(p, "buffer overflow");
    strcpy((char*) p->r, newVal);

    return OK;
}

/* strcat */

int strcat_opcode(CSOUND *csound, STRCAT_OP *p)
{
    char  *newVal1 = (char*) p->str1;
    char  *newVal2 = (char*) p->str2;

    if (UNLIKELY((int) (strlen(newVal1) + strlen(newVal2)) >= csound->strVarMaxLen))
      return StrOp_ErrMsg(p, "buffer overflow");
    if (p->r != p->str2) {
      if (p->r != p->str1)
        strcpy((char*) p->r, newVal1);
      strcat((char*) p->r, newVal2);
      return OK;
    }
    if (newVal1[0] == '\0')
      return OK;
    memmove(newVal2 + strlen(newVal1), newVal2, strlen(newVal2) + 1);
    if (p->r != p->str1)
      memcpy(newVal2, newVal1, strlen(newVal1));

    return OK;
}

/* strcmp */

int strcmp_opcode(CSOUND *csound, STRCAT_OP *p)
{
    int     i;

    (void) csound;
    *(p->r) = FL(0.0);
    if (p->str1 == p->str2)
      return OK;
    i = strcmp((char*) p->str1, (char*) p->str2);
    if (i < 0)
      *(p->r) = FL(-1.0);
    else if (i > 0)
      *(p->r) = FL(1.0);

    return OK;
}

/* perform a sprintf-style format -- based on code by Matt J. Ingalls */

static CS_NOINLINE int
    sprintf_opcode_(void *p,          /* opcode data structure pointer       */
                    char *dst,        /* pointer to space for output string  */
                    const char *fmt,  /* format string                       */
                    MYFLT **kvals,    /* array of argument pointers          */
                    int numVals,      /* number of arguments                 */
                    int strCode,      /* bit mask for string arguments       */
                    int maxLen)       /* available space in output buffer    */
{
    int     len = 0;
    char    strseg[2048], *outstring = dst;
    MYFLT   *parm = NULL;
    int     i = 0, j = 0, n;
    const char  *segwaiting = NULL;
    int     maxChars;

    if (UNLIKELY((int) ((OPDS*) p)->optext->t.xincod != 0))
      return StrOp_ErrMsg(p, "a-rate argument not allowed");
    if (UNLIKELY((int) ((OPDS*) p)->optext->t.inoffs->count > 31))
      StrOp_FatalError(p, "too many arguments");

    while (1) {
      if (UNLIKELY(i >= 2047)) {
        return StrOp_ErrMsg(p, "format string too long");
      }
      if (*fmt != '%' && *fmt != '\0') {
        strseg[i++] = *fmt++;
        continue;
      }
      if (fmt[0] == '%' && fmt[1] == '%') {
        strseg[i++] = *fmt++;   /* Odd code: %% is usually % and as we
                                   know the value of *fmt the loads are
                                   unnecessary */
        strseg[i++] = *fmt++;
        continue;
      }
      /* if already a segment waiting, then lets print it */
      if (segwaiting != NULL) {
        maxChars = maxLen - len;
        strseg[i] = '\0';
        if (UNLIKELY(numVals <= 0)) {
          return StrOp_ErrMsg(p, "insufficient arguments for format");
        }
        numVals--;
        if (UNLIKELY((*segwaiting == 's' && !(strCode & 1)) ||
                     (*segwaiting != 's' && (strCode & 1)))) {
          return StrOp_ErrMsg(p, "argument type inconsistent with format");
        }
        strCode >>= 1;
        parm = kvals[j++];
        switch (*segwaiting) {
        case 'd':
        case 'i':
        case 'o':
        case 'x':
        case 'X':
        case 'u':
        case 'c':
#ifdef HAVE_SNPRINTF
          n = snprintf(outstring, maxChars, strseg, (int) MYFLT2LRND(*parm));
#else
          n = sprintf(outstring, strseg, (int) MYFLT2LRND(*parm));
#endif
          break;
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
#ifdef HAVE_SNPRINTF
          n = snprintf(outstring, maxChars, strseg, (double)*parm);
#else
          n = sprintf(outstring, strseg, (double)*parm);
#endif
          break;
        case 's':
          if ((char*)parm == dst) {
            return StrOp_ErrMsg(p, "output argument may not be "
                                   "the same as any of the input args");
          }
#ifdef HAVE_SNPRINTF
          n = snprintf(outstring, maxChars, strseg, (char*) parm);
#else
          n = sprintf(outstring, strseg, (char*)parm);
#endif
          break;
        default:
          return StrOp_ErrMsg(p, "invalid format string");
        }
        if (UNLIKELY(n < 0 || n >= maxChars)) {
#ifdef HAVE_SNPRINTF
          /* safely detected excess string length */
          return StrOp_ErrMsg(p, "buffer overflow");
#else
          /* wrote past end of buffer - hope that did not already crash ! */
          StrOp_FatalError(p, "buffer overflow");
#endif
        }
        outstring += n;
        len += n;
        i = 0;
      }
      if (*fmt == '\0')
        break;
      /* copy the '%' */
      strseg[i++] = *fmt++;
      /* find the format code */
      segwaiting = fmt;
      while (!isalpha(*segwaiting) && *segwaiting != '\0')
        segwaiting++;
    }
    if (UNLIKELY(numVals > 0)) {
      return StrOp_ErrMsg(p, "too many arguments for format");
    }
    return 0;
}

int sprintf_opcode(CSOUND *csound, SPRINTF_OP *p)
{
    if (UNLIKELY(sprintf_opcode_(p, (char*) p->r, (char*) p->sfmt, &(p->args[0]),
                        (int) p->INOCOUNT - 1, ((int) p->XSTRCODE >> 1),
                                 csound->strVarMaxLen) != 0)) {
      ((char*) p->r)[0] = '\0';
      return NOTOK;
    }
    return OK;
}

static CS_NOINLINE int printf_opcode_(CSOUND *csound, PRINTF_OP *p)
{
    char  buf[3072];
    int   err;
    err = sprintf_opcode_(p, buf, (char*) p->sfmt, &(p->args[0]),
                          (int) p->INOCOUNT - 2, ((int) p->XSTRCODE >> 2),
                          3072);
    if (LIKELY(err == OK))
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", buf);

    return err;
}

int printf_opcode_init(CSOUND *csound, PRINTF_OP *p)
{
    if (*p->ktrig > FL(0.0))
      return (printf_opcode_(csound, p));
    return OK;
}

int printf_opcode_set(CSOUND *csound, PRINTF_OP *p)
{
    (void) csound;
    p->prv_ktrig = FL(0.0);
    return OK;
}

int printf_opcode_perf(CSOUND *csound, PRINTF_OP *p)
{
    if (*p->ktrig == p->prv_ktrig)
      return OK;
    p->prv_ktrig = *p->ktrig;
    if (p->prv_ktrig > FL(0.0))
      return (printf_opcode_(csound, p));
    return OK;
}

int puts_opcode_init(CSOUND *csound, PUTS_OP *p)
{
    p->noNewLine = (*p->no_newline == FL(0.0) ? 0 : 1);
    if (*p->ktrig > FL(0.0)) {
      if (!p->noNewLine)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*) p->str);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", (char*) p->str);
    }
    p->prv_ktrig = *p->ktrig;

    return OK;
}

int puts_opcode_perf(CSOUND *csound, PUTS_OP *p)
{
    if (*p->ktrig != p->prv_ktrig && *p->ktrig > FL(0.0)) {
      p->prv_ktrig = *p->ktrig;
      if (!p->noNewLine)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*) p->str);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", (char*) p->str);
    }

    return OK;
}

int strtod_opcode(CSOUND *csound, STRSET_OP *p)
{
    char    *s = NULL, *tmp;
    double  x;

    if (p->XSTRCODE)
      s = (char*) p->str;
    else {
      if (*p->str == SSTRCOD)
        s = csound->currevent->strarg;
      else {
        int ndx = (int) MYFLT2LRND(*p->str);
        if (ndx >= 0 && ndx <= (int) csound->strsmax && csound->strsets != NULL)
          s = csound->strsets[ndx];
      }
      if (UNLIKELY(s == NULL))
        return StrOp_ErrMsg(p, "empty string");
    }
    while (*s == ' ' || *s == '\t') s++;
    if (UNLIKELY(*s == '\0'))
      return StrOp_ErrMsg(p, "empty string");
    x = strtod(s, &tmp);
    if (UNLIKELY(*tmp != '\0'))
      return StrOp_ErrMsg(p, "invalid format");
    *p->indx = (MYFLT) x;

    return OK;
}

int strtol_opcode(CSOUND *csound, STRSET_OP *p)
{
    char  *s = NULL;
    int   sgn = 0, radix = 10;
    int32  x = 0L;

    if (p->XSTRCODE)
      s = (char*) p->str;
    else {
      if (*p->str == SSTRCOD)
        s = csound->currevent->strarg;
      else {
        int ndx = (int) MYFLT2LRND(*p->str);
        if (ndx >= 0 && ndx <= (int) csound->strsmax && csound->strsets != NULL)
          s = csound->strsets[ndx];
      }
      if (UNLIKELY(s == NULL))
        return StrOp_ErrMsg(p, "empty string");
    }
    while (*s == ' ' || *s == '\t') s++;
    if (UNLIKELY(*s == '\0'))
      return StrOp_ErrMsg(p, "empty string");
    if (*s == '+') s++;
    else if (*s == '-') sgn++, s++;
    if (*s == '0') {
      if (s[1] == 'x' || s[1] == 'X')
        radix = 16, s += 2;
      else if (s[1] != '\0')
        radix = 8, s++;
      else {
        *p->indx = FL(0.0);
        return OK;
      }
    }
    if (UNLIKELY(*s == '\0'))
      return StrOp_ErrMsg(p, "invalid format");
    switch (radix) {
      case 8:
        while (*s >= '0' && *s <= '7') x = (x * 8L) + (int32) (*s++ - '0');
        break;
      case 10:
        while (*s >= '0' && *s <= '9') x = (x * 10L) + (int32) (*s++ - '0');
        break;
      default:
        while (1) {
          if (*s >= '0' && *s <= '9')
            x = (x * 16L) + (int32) (*s++ - '0');
          else if (*s >= 'A' && *s <= 'F')
            x = (x * 16L) + (int32) (*s++ - 'A') + 10L;
          else if (*s >= 'a' && *s <= 'f')
            x = (x * 16L) + (int32) (*s++ - 'a') + 10L;
          else
            break;
        }
    }
    if (UNLIKELY(*s != '\0'))
      return StrOp_ErrMsg(p, "invalid format");
    if (sgn) x = -x;
    *p->indx = (MYFLT) x;

    return OK;
}

/**
 * Sdst    strsub      Ssrc[, istart[, iend]]
 * Sdst    strsubk     Ssrc, kstart, kend
 *
 * Extract a part of Ssrc, from istart to iend; if istart or iend is
 * less than 0, or greater than the length of the source string, it is
 * interpreted as the end of the source string. istart > iend will
 * reverse the string. The default parameters are istart = 0, iend = -1.
 */

int strsub_opcode(CSOUND *csound, STRSUB_OP *p)
{
    const char  *src;
    char        *dst;
    int         i, len, strt, end, rev = 0;

    src = (char*) p->Ssrc;
    dst = (char*) p->Sdst;
    len = (int) strlen(src);
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    strt = (int) MYFLT2LRND(*(p->istart));
    end = (int) MYFLT2LRND(*(p->iend));
#else
    strt = (int) (*(p->istart) + FL(1.5)) - 1;
    end = (int) (*(p->iend) + FL(1.5)) - 1;
#endif
    if (strt < 0 || strt > len)
      strt = len;
    if (end < 0 || end > len)
      end = len;
    if (strt == end) {
      /* trivial case: empty output */
      dst[0] = '\0';
      return OK;
    }
    if (strt > end) {
      int   tmp = strt;
      /* reverse output */
      strt = end;
      end = tmp;
      rev = 1;
    }
    src += strt;
    len = end - strt;
    if (UNLIKELY(len >= csound->strVarMaxLen)) {
      ((char*) p->Sdst)[0] = '\0';
      return StrOp_ErrMsg(p, "buffer overflow");
    }
    i = 0;
    if (!rev || p->Sdst == p->Ssrc) {
      /* copying in forward direction is safe */
      /* even if Ssrc and Sdst are the same */
      do {
        dst[i] = src[i];
      } while (++i < len);
      dst[i] = '\0';
      if (rev) {
        int   j;
        /* if the destination string variable is the same as the source, */
        /* reversing needs to be handled in a special way */
        i = 0;
        j = len - 1;
        while (i < j) {
          char  tmp = dst[i];
          dst[i++] = dst[j];
          dst[j--] = tmp;
        }
      }
    }
    else {
      /* reverse string out of place (Ssrc and Sdst are not the same) */
      int   j = len;
      do {
        dst[i] = src[--j];
      } while (++i < len);
      dst[i] = '\0';
    }

    return OK;
}

/**
 * ichr    strchar     Sstr[, ipos]
 * kchr    strchark    Sstr[, kpos]
 *
 * Return the ASCII code of the character in Sstr at ipos (defaults to 0).
 * If ipos is out of range, 0 is returned.
 */

int strchar_opcode(CSOUND *csound, STRCHAR_OP *p)
{
    int     len = (int) strlen((char*) p->Ssrc);
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    int     pos = (int) MYFLT2LRND(*(p->ipos));
#else
    int     pos = (int) (*(p->ipos) + FL(1.5)) - 1;
#endif

    (void) csound;
    if (pos < 0 || pos >= len)
      *(p->ichr) = FL(0.0);
    else
      *(p->ichr) = (MYFLT) ((int) ((unsigned char) ((char*) p->Ssrc)[pos]));

    return OK;
}

/**
 * ilen    strlen      Sstr
 * klen    strlenk     Sstr
 *
 * Return the length of a string.
 */

int strlen_opcode(CSOUND *csound, STRLEN_OP *p)
{
    (void) csound;
    *(p->ilen) = (MYFLT) ((int) strlen((char*) p->Ssrc));
    return OK;
}

/**
 * Sdst    strupper    Ssrc
 * Sdst    strupperk   Ssrc
 * Sdst    strlower    Ssrc
 * Sdst    strlowerk   Ssrc
 *
 * Convert a string to upper or lower case.
 */

int strupper_opcode(CSOUND *csound, STRUPPER_OP *p)
{
    const char  *src;
    char        *dst;
    int         i;

    (void) csound;
    src = (char*) p->Ssrc;
    dst = (char*) p->Sdst;
    for (i = 0; src[i] != '\0'; i++) {
      unsigned char   tmp;
      tmp = (unsigned char) src[i];
      dst[i] = (char) (islower(tmp) ? (unsigned char) toupper(tmp) : tmp);
    }

    return OK;
}

int strlower_opcode(CSOUND *csound, STRUPPER_OP *p)
{
    const char  *src;
    char        *dst;
    int         i;

    (void) csound;
    src = (char*) p->Ssrc;
    dst = (char*) p->Sdst;
    for (i = 0; src[i] != '\0'; i++) {
      unsigned char   tmp;
      tmp = (unsigned char) src[i];
      dst[i] = (char) (isupper(tmp) ? (unsigned char) tolower(tmp) : tmp);
    }

    return OK;
}

/**
 * Sval    getcfg      iopt
 *
 * Returns the value of a global setting (e.g. input file name) as a
 * string.
 */

int getcfg_opcode(CSOUND *csound, GETCFG_OP *p)
{
    const char  *s;
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    int         opt = (int) MYFLT2LRND(*(p->iopt));
#else
    int         opt = (int) (*(p->iopt) + FL(0.5));
#endif
    char        buf[32];

    ((char*) p->Sdst)[0] = '\0';
    buf[0] = '\0';
    s = &(buf[0]);
    switch (opt) {
    case 1:             /* maximum length of string variables */
      sprintf(&(buf[0]), "%d", (int) csound->strVarMaxLen - 1);
      break;
    case 2:             /* input sound file name */
      s = (csound->oparms->sfread && !csound->initonly ?
           csound->oparms->infilename : (char*) NULL);
      break;
    case 3:             /* output sound file name */
      s = (csound->oparms->sfwrite && !csound->initonly ?
           csound->oparms->outfilename : (char*) NULL);
      break;
    case 4:             /* is real-time audio being used ? (0: no, 1: yes) */
      buf[0] = '0';
      buf[1] = '\0';
      if ((csound->oparms->sfread && !csound->initonly &&
           check_rtaudio_name(csound->oparms->infilename, NULL, 0) >= 0) ||
          (csound->oparms->sfwrite && !csound->initonly &&
           check_rtaudio_name(csound->oparms->outfilename, NULL, 1) >= 0))
        buf[0] = '1';
      break;
    case 5:             /* is beat mode being used ? (0: no, 1: yes) */
      buf[0] = (csound->oparms->Beatmode ? '1' : '0');
      buf[1] = '\0';
      break;
    case 6:             /* host OS name */
#ifdef LINUX
      s = "Linux";
#elif defined(WIN32)
      s = "Win32";
#elif defined(MACOSX)
      s = "MacOSX";
#elif defined(mac_classic)
      s = "MacOS";
#else
      s = "unknown";
#endif
      break;
    case 7:             /* is the channel I/O callback set ? (0: no, 1: yes) */
      buf[0] = (csound->channelIOCallback_
                == (CsoundChannelIOCallback_t) NULL ? '0' : '1');
      buf[1] = '\0';
      break;
    default:
      return csound->InitError(csound, Str("invalid option code: %g"),
                                       *(p->iopt));
    }
    if (s != NULL) {
      if (UNLIKELY((int) strlen(s) >= csound->strVarMaxLen))
        return csound->InitError(csound, Str("getcfg: buffer overflow"));
      strcpy((char*) p->Sdst, s);
    }

    return OK;
}

/**
 * ipos    strindex    Sstr1, Sstr2
 * kpos    strindexk   Sstr1, Sstr2
 *
 * Return the position of the first occurence of Sstr2 in Sstr1,
 * or -1 if not found. If Sstr2 is empty, 0 is returned.
 */

int strindex_opcode(CSOUND *csound, STRINDEX_OP *p)
{
    const char  *s1 = (char*) p->Ssrc1;
    const char  *s2 = (char*) p->Ssrc2;
    int         i, j;

    (void) csound;
    /* search substring from left to right, */
    /* and return position of first match */
    i = j = 0;
    while (s2[j] != '\0') {
      if (s1[i] == '\0') {
        *(p->ipos) = -FL(1.0);
        return OK;
      }
      j = (s1[i] != s2[j] ? 0 : j + 1);
      i++;
    }
    *(p->ipos) = (MYFLT) (i - j);

    return OK;
}

/**
 * ipos    strrindex   Sstr1, Sstr2
 * kpos    strrindexk  Sstr1, Sstr2
 *
 * Return the position of the last occurence of Sstr2 in Sstr1,
 * or -1 if not found. If Sstr2 is empty, the length of Sstr1 is
 * returned.
 */

int strrindex_opcode(CSOUND *csound, STRINDEX_OP *p)
{
    const char  *s1 = (char*) p->Ssrc1;
    const char  *s2 = (char*) p->Ssrc2;
    int         i, j, k;

    (void) csound;
    /* search substring from left to right, */
    /* and return position of last match */
    i = j = 0;
    k = -1;
    while (1) {
      if (s2[j] == '\0') {
        k = i - j;
        j = 0;
      }
      if (s1[i] == '\0')
        break;
      j = (s1[i] != s2[j] ? 0 : j + 1);
      i++;
    }
    *(p->ipos) = (MYFLT) k;

    return OK;
}

