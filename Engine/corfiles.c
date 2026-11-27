/*
    corfiles.c:

    Copyright (C) 2011 John ffitch

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

#include "csoundCore.h"     /*                              CORFILES.C      */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "filesys.h"
#include "corfile.h"



CORFIL *corfile_create_w(CSOUND *csound)
{
    CORFIL *ans = (CORFIL*) csound->Malloc(csound, sizeof(CORFIL));
    ans->body = (char*)csound->Calloc(csound, 256);
    ans->len = 256;
    ans->p = 0;
    return ans;
}

CORFIL *corfile_create_r(CSOUND *csound, const char *text)
{
    //char *strdup(const char *);
    CORFIL *ans = (CORFIL*) csound->Malloc(csound, sizeof(CORFIL));
    ans->body = csoundStrdup(csound, (char*)text);
    ans->len = (int32_t) strlen(text)+1;
    ans->p = 0;
    return ans;
}

static void corfile_grow(CSOUND *csound, CORFIL *f, int32_t needed)
{
    int32_t newlen = f->len > 0 ? f->len : 256;
    while (newlen < needed) {
      if (UNLIKELY(newlen > INT32_MAX / 2)) {
        newlen = needed;
        break;
      }
      newlen *= 2;
    }
    char *newbuf = (char *) csound->ReAlloc(csound, f->body, (size_t) newlen);
    if (UNLIKELY(newbuf == NULL)) {
      csound->Die(csound, Str("Out of Memory\n"));
    }
    f->body = newbuf;
    f->len = newlen;
}

void corfile_putc(CSOUND *csound, int32_t c, CORFIL *f)
{
    if (UNLIKELY(f->p + 1 >= f->len)) {
      corfile_grow(csound, f, f->p + 2);
    }
    f->body[f->p++] = c;
    f->body[f->p] = '\0';
}

void corfile_puts(CSOUND *csound, const char *s, CORFIL *f)
{
    int32_t n;
    size_t slen;
    /* skip and count the NUL chars to the end */
    for (n=0; f->p > 0 && f->body[f->p-1] == '\0'; n++, f->p--);
    slen = strlen(s);
    if (UNLIKELY(slen > (size_t)(INT32_MAX - f->p - n - 1))) {
      csound->Die(csound, Str("Out of Memory\n"));
    }
    if (UNLIKELY(f->p + (int32_t)slen + n + 1 > f->len)) {
      corfile_grow(csound, f, f->p + (int32_t)slen + n + 1);
    }
    if (slen > 0) {
      memcpy(f->body + f->p, s, slen);
      f->p += (int32_t)slen;
    }
    while (--n >= 0) {
      f->body[f->p++] = '\0';
    }
    f->body[f->p] = '\0';
}

void corfile_flush(CSOUND *csound, CORFIL *f)
{
    char *new;
    f->len = (int32_t)  strlen(f->body)+1;
    new = (char*)csound->ReAlloc(csound, f->body, f->len);
    if (UNLIKELY(new==NULL)) {
      csound->Die(csound, Str("Out of Memory\n"));
    }
    f->body = new;
    f->body[f->len - 1] = '\0';  // VL: added NULL terminator.
    f->p = 0;
}

#undef corfile_length
int32_t corfile_length(CORFIL *f)
{
    return (int32_t)  strlen(f->body);
}

void corfile_rm(CSOUND *csound, CORFIL **ff)
{
    CORFIL *f = *ff;
    if (LIKELY(f!=NULL)) {
      csound->Free(csound, f->body);
      csound->Free(csound, f);
      *ff = NULL;
    }
}

int32_t corfile_getc(CORFIL *f)
{
    int32_t c = (unsigned char) f->body[f->p];  /* a byte 0xFF is no EOF */
    if (UNLIKELY(c=='\0')) return EOF;
    f->p++;
    return c;
}

#undef corfile_ungetc
void corfile_ungetc(CORFIL *f)
{
    --f->p;
}

cs_float corfile_get_flt(CORFIL *f)
{
    int32_t n = f->p;
    char *endptr = NULL;
    cs_float ans;
    if (UNLIKELY(f->body[n] == '\0'))
      return FL(0.0);
    ans = (cs_float) csoundStrtod(&f->body[n], &endptr);
    if (endptr != NULL && endptr > &f->body[n]) {
      f->p = (int32_t)(endptr - f->body);
    } else {
      while (f->body[f->p] != '\0' && !isspace((unsigned char)f->body[f->p]))
        f->p++;
    }
    return ans;
}

#undef corfile_rewind
void corfile_rewind(CORFIL *f)
{
    f->p = 0;
}

#undef corfile_reset
void corfile_reset(CORFIL *f)
{
    f->p = 0;
    f->body[0] = '\0';
}

#undef corfile_tell
int32_t corfile_tell(CORFIL *f)
{
    return f->p;
}

#undef corfile_set
void corfile_set(CORFIL *f, int32_t n)
{
    f->p = n;
}

void corfile_seek(CORFIL *f, int32_t n, int32_t dir)
{
    int64_t len = (int64_t) strlen(f->body);
    int64_t pos = (int64_t) f->p;
    if (dir == SEEK_SET) pos = (int64_t) n;
    else if (dir == SEEK_CUR) pos += (int64_t) n;
    else if (dir == SEEK_END) pos = len - (int64_t) n;
    if (UNLIKELY(pos < 0))
      pos = 0;
    else if (UNLIKELY(pos > len))
      pos = len;
    f->p = (uint32_t) pos;
}


#undef corfile_body
char *corfile_body(CORFIL *f)
{
    return f->body;
}

#undef corfile_current
char *corfile_current(CORFIL *f)
{
    return f->body+f->p;
}

/* *** THIS NEEDS TO TAKE ACCOUNT OF SEARCH PATH *** */
CORFIL *copy_to_corefile(CSOUND *csound, const char *fname,
                         const char *env, int32_t fromScore)
{
    CORFIL *mm;
    FILE *ff;
    void *fd;
    int32_t n;
    char buffer[1024];
    if (UNLIKELY(fname==NULL)) {
      csound->ErrorMsg(csound, Str("Null file name in copy_to_corefile"));
      csound->LongJmp(csound, 1);
    }
#ifdef HAVE_CURL
    if (strstr(fname,"://")) {
      /* Strip optional ./ prefix only when present */
      const char *url = (strncmp(fname, "./", 2) == 0) ? fname + 2 : fname;
      return copy_url_corefile(csound, url, fromScore);
    }
#endif
    fd = fopen_path(csound, &ff, (const char *)fname, NULL, (char *)env, fromScore);
    if (UNLIKELY(ff==NULL)) return NULL;
    mm = corfile_create_w(csound);
    memset(buffer, '\0', 1024);
    while ((n = (int32_t)  fread(buffer, 1, 1023, ff))) {
      /* Need to lose \r characters  here */
      /* while ((s = strchr(buffer, '\r'))) { */
      /*   int32_t k = n - (s-buffer); */
      /*   memmove(s, s+1, k); */
      /*   n--; */
      /* } */
      corfile_puts(csound, buffer, mm);
      memset(buffer, '\0', 1024);
    }
    if (fromScore) {
      corfile_puts(csound, "\ne\n#exit\n", mm);
    }
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    if (fromScore) corfile_flush(csound, mm);
    csoundFileClose(csound, fd, CSFILE_CLOSE_SYNC);
    //if (fromScore) printf("Copy is >>%s<<\n", mm->body);
    return mm;
}

void corfile_preputs(CSOUND *csound, const char *s, CORFIL *f)
{
    char *body = f->body;
    f->body = (char*)csound->Malloc(csound, f->len=(int32_t) (strlen(body)+strlen(s)+1));
    f->p = f->len-1;
    strcpy(f->body, s); strcat(f->body, body);
    csound->Free(csound, body);
}


CORFIL *copy_string_to_corefile(CSOUND *csound, const char *string,
                                int32_t fromScore){
    CORFIL *mm;
    if (UNLIKELY(string==NULL)) {
      csound->ErrorMsg(csound, Str("Null string"));
      csound->LongJmp(csound, 1);
    }
    mm = corfile_create_w(csound);
    corfile_puts(csound, string, mm);
    if (fromScore) {
      corfile_puts(csound, "\ne\n#exit\n", mm);
    }
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    if (fromScore) corfile_flush(csound, mm);
    return mm;
}


#ifdef HAVE_CURL

#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
  CSOUND* cs;
};


static size_t
write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  CSOUND *csound = mem->cs;
  char *ptr = csound->ReAlloc(csound, mem->memory, mem->size + realsize + 1);
  if (UNLIKELY(ptr == NULL)) {
    /* out of memory! */
    csound->ErrorMsg(csound, Str("not enough memory (realloc returned NULL)\n"));
    return 0;
  }
  mem->memory = ptr;

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

CORFIL *copy_url_corefile(CSOUND *csound, const char *url, int32_t fromScore)
{
    int32_t n;
    CURL *curl = curl_easy_init();
    CORFIL *mm;
    struct MemoryStruct chunk;

    if (UNLIKELY(curl == NULL)) {
      csound->ErrorMsg(csound, Str("curl_easy_init() failed\n"));
      return NULL;
    }
    mm = corfile_create_w(csound);
    chunk.memory = csound->Malloc(csound, 1);  /* will grown */
    chunk.size = 0;    /* no data at this point */
    chunk.cs = csound;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    n = curl_easy_perform(curl);
    if (UNLIKELY(n != CURLE_OK)) {
      curl_easy_cleanup(curl);
      csound->Free(csound, chunk.memory);
      corfile_rm(csound, &mm);
      csound->Die(csound, Str("curl_easy_perform() failed: %s\n"),
                  curl_easy_strerror(n));
    }
    curl_easy_cleanup(curl);
    corfile_puts(csound, chunk.memory, mm);
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    if (fromScore) corfile_flush(csound, mm);
    csound->Free(csound, chunk.memory);

    return mm;
}

#endif
