/*
    str_ops.c:

    Copyright (C) 2005, 2006 Istvan Varga
              (C) 2005       Matt J. Ingalls, John ffitch
              (C) 2013   V Lazzarini (new string code)

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

#include "csoundCore.h"
#define CSOUND_STR_OPS_C    1
#include "str_ops.h"
#include <ctype.h>
#ifdef HAVE_CURL
#include <curl/curl.h>
#include "corfile.h"
#endif

#define STRSMAX 8

#ifndef HAVE_SNPRINTF
/* add any compiler/system that has snprintf() */
#if defined(HAVE_C99)
#define HAVE_SNPRINTF   1
#endif
#endif

int32_t s_opcode(CSOUND *csound, STRGET_OP *p){
    if (p->r->data == NULL){
      p->r->data = (char *) csound->Malloc(csound, 15);
      p->r->size = 15;
    } else if (p->r->size < 15){
      p->r->data = (char *) csound->ReAlloc(csound, p->r->data, 15);
      p->r->size = 15;
    }
    snprintf(p->r->data, p->r->size, "%f", *p->indx);
    return OK;
}

int32_t s_opcode_k(CSOUND *csound, STRGET_OP *p){
     IGN(csound);
    snprintf(p->r->data, p->r->size, "%f", *p->indx);
    return OK;
}

/* strset by John ffitch */

static void str_set(CSOUND *csound, int32_t ndx, const char *s)
{
    if (UNLIKELY(csound->strsets == NULL)) {
      csound->strsmax = STRSMAX;
      csound->strsets = (char **) csound->Calloc(csound, (csound->strsmax + 1)
                                                         * sizeof(char*));
    }
    if (UNLIKELY(ndx > (int32_t) csound->strsmax)) {
      int32_t   i, newmax;
      /* assumes power of two STRSMAX */
      newmax = (ndx | (STRSMAX - 1)) + 1;
      csound->strsets = (char**) csound->ReAlloc(csound, csound->strsets,
                                                 (newmax + 1) * sizeof(char*));
      for (i = (csound->strsmax + 1); i <= newmax; i++)
        csound->strsets[i] = NULL;
      csound->strsmax = newmax;
    }
    if (UNLIKELY(ndx < 0))  {  /* -ve index */
      csound->InitError(csound, Str("illegal strset index"));
      return;
    }

    if (csound->strsets[ndx] != NULL) {
      if (strcmp(s, csound->strsets[ndx]) == 0)
        return;
      if (UNLIKELY(csound->oparms->msglevel & CS_WARNMSG)) {
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

int32_t strset_init(CSOUND *csound, STRSET_OP *p)
{
    str_set(csound, (int32_t) MYFLT2LRND(*p->indx), p->str->data);
    return OK;
}

/* for argdecode.c */

void strset_option(CSOUND *csound, char *s)
{
    int32_t indx = 0;

    if (UNLIKELY(!isdigit(*s))) {
       csound->Warning(csound, Str("--strset: invalid format"));
       return;
    }
    do {
      indx = (indx * 10) + (int32_t) (*s++ - '0');
    } while (isdigit(*s));
    if (UNLIKELY(*s++ != '=')){
      csound->Warning(csound, Str("--strset: invalid format"));
      return;
    }
    str_set(csound, indx, s);
}

int32_t strget_init(CSOUND *csound, STRGET_OP *p)
{
    int32_t   indx;
    if (csound->ISSTRCOD(*(p->indx))) {
      char *ss = csound->init_event->strarg;
      if (ss == NULL)
        return OK;
      ss = get_arg_string(csound, *p->indx);
      if (p->r->data == NULL) {
        p->r->data = cs_strdup(csound, ss);
        p->r->size = strlen(ss)+1;
       }
      else if ((int32_t) strlen(ss) >= p->r->size) {
        csound->Free(csound, p->r->data);
        p->r->data = cs_strdup(csound, ss);
        p->r->size = strlen(ss) + 1;
      }
      else {
        int32_t n = strlen(ss)+1;
        p->r->size = n;
        strNcpy(p->r->data, ss, n);
        //p->r->data[p->r->size - 1] = '\0';
      }
      return OK;
    }
    indx = (int32_t)((double)*(p->indx) + (*(p->indx) >= FL(0.0) ? 0.5 : -0.5));
    if (indx < 0 || indx > (int32_t) csound->strsmax ||
        csound->strsets == NULL || csound->strsets[indx] == NULL)
      return OK;
    if (UNLIKELY((int32_t) strlen(csound->strsets[indx]) >= p->r->size)){
      int32_t size = strlen(csound->strsets[indx]);
      p->r->data = csound->ReAlloc(csound, p->r->data, size + 1);
      p->r->size = size + 1;
    }
    strcpy((char*) p->r->data, csound->strsets[indx]);
    return OK;
}

static CS_NOINLINE int32_t StrOp_ErrMsg(void *p, const char *msg)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *opname = csound->GetOpcodeName(p);

    if (UNLIKELY(csound->ids != NULL && csound->ids->insdshead == csound->curip))
      return csound->InitError(csound, "%s: %s", opname, Str(msg));
    else if (UNLIKELY(((OPDS*) p)->insdshead->pds != NULL))
      return csound->PerfError(csound, (OPDS*)p,
                               "%s: %s", opname, Str(msg));
    else
      csound->Warning(csound, "%s: %s", opname, Str(msg));

    return NOTOK;
}
/* strcpy */
int32_t strcpy_opcode_S(CSOUND *csound, STRCPY_OP *p)
{
    char  *newVal = p->str->data;
    if (p->r->data == NULL) {
      p->r->data =  cs_strdup(csound, newVal);
      p->r->size =  strlen(p->str->data) + 1;
      //printf("NULL str:%p %p \n", p->r, p->r->data);
        return OK;
    }
    if (p->r->data == p->str->data){
      //printf("sameptr str:%p %p \n", p->r->data);
      return OK;
    }
    if (UNLIKELY((int32_t) strlen(newVal) >= p->r->size)){
        csound->Free(csound, p->r->data);
        p->r->data = cs_strdup(csound, newVal);
        p->r->size = strlen(newVal) + 1;
        //printf("dup str:%p %p \n", p->r, p->r->data);
    }
    else {
      strcpy((char*) p->r->data, newVal);
      p->r->size = strlen(newVal) + 1;
      //printf("str:%p %p \n", p->r, p->r->data);
    }

    return OK;
}

int32_t strassign_opcode_S(CSOUND *csound, STRCPY_OP *p)
{
   IGN(csound);
    p->r->data = p->str->data;
    p->r->size = p->str->size;
    return OK;
}
int32_t strassign_opcode_Sk(CSOUND *csound, STRCPY_OP *p)
{
    IGN(csound);
    if (strcmp(p->r->data, p->str->data)!=0){
      p->r->data = p->str->data;
      p->r->size = p->str->size;
    }
    //csound->Message(csound, p->r->data);
    return OK;
}

int32_t str_changed(CSOUND *csound, STRCHGD *p)
{
    if (p->mem != NULL)
      csound->Free(csound, p->mem);
    p->mem = cs_strdup(csound, p->str->data);
    *p->r = 0;
    return OK;
}

int32_t str_changed_k(CSOUND *csound, STRCHGD *p)
{
  if (p->str->data && ( p->mem == NULL || strcmp(p->str->data, p->mem)!=0)) {
      csound->Free(csound, p->mem);
      p->mem = cs_strdup(csound, p->str->data);
      *p->r = 1;
    }
    else *p->r = 0;
    return OK;
}

extern char* get_strarg(CSOUND *csound, MYFLT p, char *strarg);
int32_t strcpy_opcode_p(CSOUND *csound, STRGET_OP *p)
{
    if (csound->ISSTRCOD(*p->indx)) {
      char *ss;
      ss = get_arg_string(csound, *p->indx);
      if (ss == NULL){
      if (UNLIKELY(((OPDS*) p)->insdshead->pds != NULL))
        return csoundPerfError(csound, (OPDS*)p,
                               Str("NULL string\n"));
      else
        return csoundInitError(csound, Str("NULL string\n"));
    }
      if (p->r->data == NULL) {
        p->r->data = cs_strdup(csound, ss);
        p->r->size = strlen(ss)+1;
      }
      else if ((int32_t) strlen(ss) >= p->r->size) {
        csound->Free(csound, p->r->data);
        p->r->data = cs_strdup(csound, ss);
        p->r->size = strlen(ss) + 1;
      }
      else {
        strcpy(p->r->data,ss);
        p->r->size = strlen(ss) + 1;
      }
    }
    else {
      p->r->data = csound->strarg2name(csound, NULL, p->indx, "soundin.", 0);
      p->r->size = strlen(p->r->data) + 1;
    }

    return OK;
}


/* strcat */
int32_t strcat_opcode(CSOUND *csound, STRCAT_OP *p)
{
    int32_t size;
    char *str1 = cs_strdup(csound, p->str1->data),
         *str2 = cs_strdup(csound, p->str2->data);

    if (str1 == NULL || str2 == NULL){
      csound->Free(csound,str1);
      csound->Free(csound,str2);
      if (UNLIKELY(((OPDS*) p)->insdshead->pds != NULL))
        return csoundPerfError(csound, (OPDS*)p, Str("NULL string\n"));
      else return csoundInitError(csound, Str("NULL string\n"));
    }

    size = strlen(str1) + strlen(str2);

    if (p->r->data == NULL) {
        p->r->data = csound->Calloc(csound, size+1);
        p->r->size = size+1;
    }
    else if (UNLIKELY((int32_t) size >= p->r->size)) {
       char *nstr =  csound->ReAlloc(csound, p->r->data, size + 1);
       if (p->r->data == p->str1->data){
         p->str1->data = nstr;
         p->str1->size = size + 1;
       }
       if (p->r->data == p->str2->data){
         p->str2->data = nstr;
         p->str2->size = size + 1;
       }
         p->r->data = nstr;
         p->r->size = size + 1;
    }

    strNcpy((char*) p->r->data,  str1, p->r->size);
    strcat((char*) p->r->data, str2);

    csound->Free(csound, str2);                 /* not needed anymore */
    csound->Free(csound, str1);
    return OK;
}

/* strcmp */

int32_t strcmp_opcode(CSOUND *csound, STRCMP_OP *p)
{
    int32_t     i;
    if (p->str1->data == NULL || p->str2->data == NULL){
      if (UNLIKELY(((OPDS*) p)->insdshead->pds != NULL))
        return csoundPerfError(csound, (OPDS*)p, Str("NULL string\n"));
      else return csoundInitError(csound, Str("NULL string\n"));
    }

    *(p->r) = FL(0.0);
    if (p->str1 == p->str2)
      return OK;
    i = strcmp((char*) p->str1->data, (char*) p->str2->data);
    if (i < 0)
      *(p->r) = FL(-1.0);
    else if (i > 0)
      *(p->r) = FL(1.0);

    return OK;
}

/* perform a sprintf-style format -- based on code by Matt J. Ingalls */

static CS_NOINLINE int32_t
sprintf_opcode_(CSOUND *csound,
                    void *p,          /* opcode data structure pointer       */
                    STRINGDAT *str,   /* pointer to space for output string  */
                    const char *fmt,  /* format string                       */
                    MYFLT **kvals,    /* array of argument pointers          */
                    int32_t numVals,      /* number of arguments             */
                    int32_t strCode)      /* bit mask for string arguments   */
{
    int32_t     len = 0;
    char    *strseg, *outstring = str->data;
    MYFLT   *parm = NULL;
    int32_t     i = 0, j = 0, n;
    const char  *segwaiting = NULL;
    int32_t     maxChars, siz = strlen(fmt) + numVals*7 + 1;

    for (i = 0; i < numVals; i++) {
      if (UNLIKELY(IS_ASIG_ARG(kvals[i]))) {
        return StrOp_ErrMsg(p, Str("a-rate argument not allowed"));
      }
    }

    if (UNLIKELY((int32_t) ((OPDS*) p)->optext->t.inArgCount > 31)){
      StrOp_ErrMsg(p, Str("too many arguments"));
      return NOTOK;
    }
    if (numVals==0) {
      strcpy(str->data, fmt);
      return OK;
    }

    strseg = csound->Malloc(csound, siz);
    i = 0;

    while (1) {
      if (UNLIKELY(i >= siz)) {
        // return StrOp_ErrMsg(p, "format string too long");
        siz *= 2;
        strseg = csound->ReAlloc(csound, strseg, siz);
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

        maxChars = str->size - len;
        strseg[i] = '\0';
        if (UNLIKELY(numVals <= 0)) {
          csound->Free(csound, strseg);
          return StrOp_ErrMsg(p, Str("insufficient arguments for format"));
        }
        numVals--;
        /* if (UNLIKELY((*segwaiting == 's' && !(strCode & 1)) || */
        /*              (*segwaiting != 's' && (strCode & 1)))) { */
        /*   return StrOp_ErrMsg(p, "argument type inconsistent with format"); */
        /* } */
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
          if ((int32_t)strlen(strseg) + 24 > (int32_t)maxChars) {
            int32_t offs = outstring - str->data;
            str->data = csound->ReAlloc(csound, str->data,
                                 str->size  + 24);
            if(str->data == NULL) {
              return StrOp_ErrMsg(p, Str("memory allocation failure"));
            }
            str->size += 24;
            maxChars += 24;
            outstring = str->data + offs;
            //printf("maxchars = %d  %s\n", maxChars, strseg);
            //printf("size: %d \n",str->size);

          }
          n = snprintf(outstring, maxChars, strseg, (int32_t) MYFLT2LRND(*parm));
#else
          n = sprintf(outstring, strseg, (int32_t) MYFLT2LRND(*parm));
#endif
          break;
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
#ifdef HAVE_SNPRINTF
          //printf("%d %d \n", str->size, strlen(str->data));
          if (strlen(strseg) + 24 > (uint32_t)maxChars) {
            int32_t offs = outstring - str->data;
            str->data = csound->ReAlloc(csound, str->data, str->size  + 13);
            if(str->data == NULL) {
              return StrOp_ErrMsg(p, Str("memory allocation failure"));
            }
            str->size += 24;
            maxChars += 24;
            outstring = str->data + offs;
            //printf("maxchars = %d  %s\n", maxChars, strseg);
          }
          //printf("%d %d \n", str->size, strlen(str->data));
          n = snprintf(outstring, maxChars, strseg, (double)*parm);
#else
          n = sprintf(outstring, strseg, (double)*parm);
#endif
          break;
        case 's':
          if (LIKELY(IS_STR_ARG(parm))) {
            if (UNLIKELY(((STRINGDAT*)parm)->data == str->data)) {
              csound->Free(csound, strseg);
              return StrOp_ErrMsg(p, Str("output argument may not be "
                                         "the same as any of the input args"));
            }
            if (UNLIKELY(((STRINGDAT*)parm)->size+strlen(strseg) >= (uint32_t)maxChars)) {
              int32_t offs = outstring - str->data;
              str->data = csound->ReAlloc(csound, str->data,
                                          str->size  + ((STRINGDAT*)parm)->size +
                                          strlen(strseg));
              if(str->data == NULL){
                return StrOp_ErrMsg(p, Str("memory allocation failure"));
              }
              str->size += ((STRINGDAT*)parm)->size + strlen(strseg);
              maxChars += ((STRINGDAT*)parm)->size + strlen(strseg);
              outstring = str->data + offs;
            }
            n = snprintf(outstring, maxChars, strseg, ((STRINGDAT*)parm)->data);
          }
          else return StrOp_ErrMsg(p, Str("Not string type for %%s"));
          break;
        default:
          csound->Free(csound, strseg);
          return StrOp_ErrMsg(p, Str("invalid format string"));
        }
        if (n < 0 || n >= maxChars) {
          /* safely detected excess string length */
            int32_t offs = outstring - str->data;
            str->data = csound->ReAlloc(csound, str->data, maxChars*2);
            if (str->data == NULL) {
              return StrOp_ErrMsg(p, Str("memory allocation failure"));
            }
            outstring = str->data + offs;
            str->size = maxChars*2;
            // VL: Coverity says this is unused. (which is true)
            // maxChars += str->size;

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
      csound->Free(csound, strseg);
      return StrOp_ErrMsg(p, Str("too many arguments for format"));
    }
    csound->Free(csound, strseg);
    return OK;
}

int32_t sprintf_opcode(CSOUND *csound, SPRINTF_OP *p)
{
    int32_t size = p->sfmt->size+ 18*((int32_t) p->INOCOUNT);
    //printf("%d %d \n", p->r->size, strlen(p->r->data));
    if (p->r->data == NULL || p->r->size < size) {
      /* this 10 is 1n incorrect guess which is OK with numbers*/
      p->r->data = csound->Calloc(csound, size);
      p->r->size = size;
    }
    if (UNLIKELY(sprintf_opcode_(csound, p, p->r,
                                  (char*) p->sfmt->data, &(p->args[0]),
                                  (int32_t) p->INOCOUNT - 1,0) == NOTOK)) {
      ((char*) p->r->data)[0] = '\0';
      return NOTOK;
    }
    return OK;
}

static CS_NOINLINE int32_t printf_opcode_(CSOUND *csound, PRINTF_OP *p)
{
    STRINGDAT buf;
    int32_t   err;
    buf.size = /*strlen(p->sfmt->data) +*/ 3072;
    buf.data = csound->Calloc(csound, buf.size);

    err = sprintf_opcode_(csound, p, &buf, (char*) p->sfmt->data, &(p->args[0]),
                          (int32_t) p->INOCOUNT - 2,0);
    if (LIKELY(err == OK))
      csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", buf.data);
    csound->Free(csound, buf.data);

    return err;
}

int32_t printf_opcode_init(CSOUND *csound, PRINTF_OP *p)
{
    if (*p->ktrig > FL(0.0))
      return (printf_opcode_(csound, p));
    return OK;
}

int32_t printf_opcode_set(CSOUND *csound, PRINTF_OP *p)
{
    (void) csound;
    p->prv_ktrig = FL(0.0);
    return OK;
}

int32_t printf_opcode_perf(CSOUND *csound, PRINTF_OP *p)
{
    MYFLT ktrig = *p->ktrig;
    if (ktrig == p->prv_ktrig)
      return OK;
    p->prv_ktrig = ktrig;
    if (ktrig > FL(0.0))
      return (printf_opcode_(csound, p));
    return OK;
}

int32_t puts_opcode_init(CSOUND *csound, PUTS_OP *p)
{
    p->noNewLine = (*p->no_newline == FL(0.0) ? 0 : 1);
    if (*p->ktrig > FL(0.0)) {
      if (!p->noNewLine)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*) p->str->data);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", (char*) p->str->data);
    }
    p->prv_ktrig = *p->ktrig;

    return OK;
}

int32_t puts_opcode_perf(CSOUND *csound, PUTS_OP *p)
{
    if (*p->ktrig != p->prv_ktrig && *p->ktrig > FL(0.0)) {
      p->prv_ktrig = *p->ktrig;
      if (!p->noNewLine)
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*) p->str->data);
      else
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s", (char*) p->str->data);
    }

    return OK;
}

int32_t strtod_opcode_p(CSOUND *csound, STRTOD_OP *p)
{
    char    *s = NULL, *tmp;
    double  x;

    if (csound->ISSTRCOD(*p->str))
      s = get_arg_string(csound, *p->str);
    else {
      int32_t ndx = (int32_t) MYFLT2LRND(*p->str);
      if (ndx >= 0 && ndx <= (int32_t) csound->strsmax && csound->strsets != NULL)
        s = csound->strsets[ndx];
    }
    if (UNLIKELY(s == NULL))
      return StrOp_ErrMsg(p, Str("empty string"));
    while (isblank(*s)) s++;
    if (UNLIKELY(*s == '\0'))
     return StrOp_ErrMsg(p, Str("empty string"));
    x = cs_strtod(s, &tmp);
    if (UNLIKELY(*tmp != '\0'))
      return StrOp_ErrMsg(p, Str("invalid format"));
    *p->indx = (MYFLT) x;

    return OK;
}

int32_t strtod_opcode_S(CSOUND *csound, STRSET_OP *p)
{
    IGN(csound);
    char    *s = NULL, *tmp;
    double  x;
    s = (char*) p->str->data;
    while (isblank(*s)) s++;
    if (UNLIKELY(*s == '\0'))
     return StrOp_ErrMsg(p, Str("empty string"));
    x = cs_strtod(s, &tmp);
    if (UNLIKELY(*tmp != '\0'))
     return StrOp_ErrMsg(p, Str("invalid format"));
    *p->indx = (MYFLT) x;

    return OK;
}

int32_t strtol_opcode_S(CSOUND *csound, STRSET_OP *p)
{
    IGN(csound);
    char  *s = NULL;
    int32_t   sgn = 0, radix = 10;
    int32_t  x = 0;

    s = (char*) p->str->data;
    while (isblank(*s)) s++;
    if (UNLIKELY(*s == '\0'))
      return StrOp_ErrMsg(p, Str("empty string"));
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
      return StrOp_ErrMsg(p, Str("invalid format"));
    switch (radix) {
      case 8:
        while (*s >= '0' && *s <= '7') x = (x * 8L) + (int32_t) (*s++ - '0');
        break;
      case 10:
        while (isdigit(*s)) x = (x * 10L) + (int32_t) (*s++ - '0');
        break;
      default:
        while (1) {
          if (isdigit(*s))
            x = (x * 16L) + (int32_t) (*s++ - '0');
          else if (*s >= 'A' && *s <= 'F')
            x = (x * 16L) + (int32_t) (*s++ - 'A') + 10L;
          else if (*s >= 'a' && *s <= 'f')
            x = (x * 16L) + (int32_t) (*s++ - 'a') + 10L;
          else
            break;
        }
    }
    if (UNLIKELY(*s != '\0'))
      return StrOp_ErrMsg(p, Str("invalid format"));
    if (sgn) x = -x;
    *p->indx = (MYFLT) x;

    return OK;
}


int32_t strtol_opcode_p(CSOUND *csound, STRTOD_OP *p)
{
    char  *s = NULL;
    int32_t   sgn = 0, radix = 10;
    int32_t   x = 0L;

    if (csound->ISSTRCOD(*p->str))
        s = get_arg_string(csound, *p->str);
      else {
        int32_t ndx = (int32_t) MYFLT2LRND(*p->str);
        if (ndx >= 0 && ndx <= (int32_t) csound->strsmax && csound->strsets != NULL)
          s = csound->strsets[ndx];
      }
      if (UNLIKELY(s == NULL))
        return StrOp_ErrMsg(p, Str("empty string"));

      while (isblank(*s)) s++;
    if (UNLIKELY(*s == '\0'))
      return StrOp_ErrMsg(p, Str("empty string"));
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
      return StrOp_ErrMsg(p, Str("invalid format"));
    switch (radix) {
      case 8:
        while (*s >= '0' && *s <= '7') x = (x * 8L) + (int32_t) (*s++ - '0');
        break;
      case 10:
        while (isdigit(*s)) x = (x * 10L) + (int32_t) (*s++ - '0');
        break;
      default:
        while (1) {
          if (isdigit(*s))
            x = (x * 16L) + (int32_t) (*s++ - '0');
          else if (*s >= 'A' && *s <= 'F')
            x = (x * 16L) + (int32_t) (*s++ - 'A') + 10L;
          else if (*s >= 'a' && *s <= 'f')
            x = (x * 16L) + (int32_t) (*s++ - 'a') + 10L;
          else
            break;
        }
    }
    if (UNLIKELY(*s != '\0'))
      return StrOp_ErrMsg(p, Str("invalid format"));
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

int32_t strsub_opcode(CSOUND *csound, STRSUB_OP *p)
{
    const char  *src;
    char        *dst;
    int32_t         i, len, strt, end, rev = 0;

    if (p->Ssrc->data == NULL) return NOTOK;
    if (p->Sdst->data == NULL || p->Sdst->size < p->Ssrc->size) {
      int32_t size = p->Ssrc->size;
      if (p->Sdst->data != NULL) csound->Free(csound, p->Sdst->data);
      p->Sdst->data = csound->Calloc(csound, size);
      p->Sdst->size = size;
    }

    src = (char*) p->Ssrc->data;
    dst = (char*) p->Sdst->data;
    len = (int32_t) strlen(src);
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    strt = (int32_t) MYFLT2LRND(*(p->istart));
    end = (int32_t) MYFLT2LRND(*(p->iend));
#else
    strt = (int32_t) (*(p->istart) + FL(1.5)) - 1;
    end = (int32_t) (*(p->iend) + FL(1.5)) - 1;
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
      int32_t   tmp = strt;
      /* reverse output */
      strt = end;
      end = tmp;
      rev = 1;
    }

    src += strt;
    len = end - strt;
    if (UNLIKELY(len >=  p->Sdst->size)) {
      p->Sdst->data = csound->ReAlloc(csound, p->Sdst->data, len+1);
      p->Sdst->size = len+1;
      dst = (char*) p->Sdst->data;
    }
    i = 0;
    if (!rev || p->Sdst->data == p->Ssrc->data) {
      /* copying in forward direction is safe */
      /* even if Ssrc and Sdst are the same */
      do {
        dst[i] = src[i];
      } while (++i < len);
      dst[i] = '\0';
      if (rev) {
        int32_t   j;
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
      int32_t   j = len;
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

int32_t strchar_opcode(CSOUND *csound, STRCHAR_OP *p)
{
    int32_t     len = (int32_t) strlen((char*) p->Ssrc->data);
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    int32_t     pos = (int32_t) MYFLT2LRND(*(p->ipos));
#else
    int32_t     pos = (int32_t) (*(p->ipos) + FL(1.5)) - 1;
#endif

    (void) csound;
    if (pos < 0 || pos >= len)
      *(p->ichr) = FL(0.0);
    else
      *(p->ichr) = (MYFLT) ((int32_t)((unsigned char)((char*) p->Ssrc->data)[pos]));

    return OK;
}

/**
 * ilen    strlen      Sstr
 * klen    strlenk     Sstr
 *
 * Return the length of a string.
 */

int32_t strlen_opcode(CSOUND *csound, STRLEN_OP *p)
{
    (void) csound;
    if (p->Ssrc->size)
      *(p->ilen) = (MYFLT) strlen(p->Ssrc->data);
    else *(p->ilen) = FL(0.0);
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

int32_t strupper_opcode(CSOUND *csound, STRUPPER_OP *p)
{
    const char  *src;
    char        *dst;
    int32_t         i;
    if (p->Ssrc->data == NULL) return NOTOK;
    if (p->Sdst->data == NULL || p->Sdst->size < p->Ssrc->size) {
      int32_t size = p->Ssrc->size;
      if (p->Sdst->data != NULL) csound->Free(csound, p->Sdst->data);
      p->Sdst->data = csound->Calloc(csound, size);
      p->Sdst->size = size;
    }

    (void) csound;
    src = (char*) p->Ssrc->data;
    dst = (char*) p->Sdst->data;
    for (i = 0; src[i] != '\0'; i++) {
      unsigned char   tmp;
      tmp = (unsigned char) src[i];
      dst[i] = (char) (islower(tmp) ? (unsigned char) toupper(tmp) : tmp);
    }

    return OK;
}

int32_t strlower_opcode(CSOUND *csound, STRUPPER_OP *p)
{
    const char  *src;
    char        *dst;
    int32_t         i;
    if (p->Ssrc->data == NULL) return NOTOK;
    if (p->Sdst->data == NULL || p->Sdst->size < p->Ssrc->size) {
      int32_t size = p->Ssrc->size;
      if (p->Sdst->data != NULL) csound->Free(csound,p->Sdst->data);
      p->Sdst->data = csound->Calloc(csound, size);
      p->Sdst->size = size;
    }

    (void) csound;
    src = (char*) p->Ssrc->data;
    dst = (char*) p->Sdst->data;
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

int32_t getcfg_opcode(CSOUND *csound, GETCFG_OP *p)
{
    const char  *s;
#if defined(MSVC) || (defined(__GNUC__) && defined(__i386__))
    int32_t         opt = (int32_t) MYFLT2LRND(*(p->iopt));
#else
    int32_t         opt = (int32_t) (*(p->iopt) + FL(0.5));
#endif
    char        buf[32];

    if (p->Sdst->size < 32){
      csound->Free(csound, p->Sdst->data);
      p->Sdst->data = csound->Calloc(csound,32);
      p->Sdst->size = 32;
    }
    //((char*) p->Sdst->data)[0] = '\0';
    buf[0] = '\0';
    s = &(buf[0]);
    switch (opt) {
    case 1:             /* maximum length of variable */
      snprintf(&(buf[0]), 32, "%d", (int32_t) p->Sdst->size - 1);
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
#else
      s = "unknown";
#endif
      break;
    case 7:             /* is the channel I/O callback set ? (0: no, 1: yes) */
      buf[0] = 0;
      buf[1] = '\0';
      break;
    default:
      return csound->InitError(csound, Str("invalid option code: %g"),
                                       *(p->iopt));
    }
    if (s != NULL) {

      if (p->Sdst->data == NULL) {
        int32_t size = strlen(s) + 1;
        p->Sdst->data = csound->Calloc(csound, size);
        p->Sdst->size = size;
      }
      else if (UNLIKELY((int32_t) strlen(s) >=  p->Sdst->size)) {
        p->Sdst->data = csound->ReAlloc(csound, p->Sdst->data, strlen(s) + 1);
        p->Sdst->size = strlen(s) + 1;
      }
      strcpy((char*) p->Sdst->data, s);
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

int32_t strindex_opcode(CSOUND *csound, STRINDEX_OP *p)
{
    const char  *s1 = (char*) p->Ssrc1->data;
    const char  *s2 = (char*) p->Ssrc2->data;
    int32_t         i, j;

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

int32_t strrindex_opcode(CSOUND *csound, STRINDEX_OP *p)
{
    const char  *s1 = (char*) p->Ssrc1->data;
    const char  *s2 = (char*) p->Ssrc2->data;
    int32_t         i, j, k;

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

#ifdef HAVE_CURL
int32_t str_from_url(CSOUND *csound, STRCPY_OP *p)
{
    char  *newVal = p->str->data;
    if (strstr(newVal, ":/")==NULL) return strcpy_opcode_S(csound, p);
    {
      CORFIL *mm = copy_url_corefile(csound, newVal,0);
      int32_t len = corfile_length(mm);
      if (p->r->data == NULL) {
        p->r->data =  cs_strdup(csound, corfile_body(mm));
        p->r->size =  len + 1;
        goto cleanup;
      }
      if (UNLIKELY(len >= p->r->size)) {
        csound->Free(csound, p->r->data);
        p->r->data = cs_strdup(csound, corfile_body(mm));
        p->r->size = len + 1;
      }
      else strcpy((char*) p->r->data, corfile_body(mm));
    cleanup:
      corfile_rm(csound, &mm);
      return OK;
    }
}
#endif

#if !defined(HAVE_STRLCAT) && !defined(strlcat) && !defined(EMSCRIPTEN)
/* Direct from BSD sources */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
      d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
      return (dlen + strlen(s));
    while (*s != '\0') {
      if (n != 1) {
        *d++ = *s;
        n--;
      }
      s++;
    }
    *d = '\0';

    return (dlen + (s - src));  /* count does not include NUL */
}
#endif

/* Modified from BSD sources for strlcpy */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/* modifed for speed -- JPff */
char *
strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\0';                /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}

/* Debugging opcode for testing runtime type identification */
int32_t print_type_opcode(CSOUND* csound, PRINT_TYPE_OP* p) {
    char* ptr = (char*)p->inVar;

    CS_TYPE* varType = *(CS_TYPE**)(ptr - CS_VAR_TYPE_OFFSET);
    csound->Message(csound, "Variable Type: %s\n", varType->varTypeName);

    return OK;
}
