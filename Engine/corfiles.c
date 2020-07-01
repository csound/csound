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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"     /*                              CORFILES.C      */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>


extern int csoundFileClose(CSOUND*, void*);
CORFIL *copy_url_corefile(CSOUND *, const char *, int);

CORFIL *corfile_create_w(CSOUND *csound)
{
    CORFIL *ans = (CORFIL*) csound->Malloc(csound, sizeof(CORFIL));
    ans->body = (char*)csound->Calloc(csound,100); /* 100 is just a number */
    ans->len = 100;
    ans->p = 0;
    return ans;
}

CORFIL *corfile_create_r(CSOUND *csound, const char *text)
{
    //char *strdup(const char *);
    CORFIL *ans = (CORFIL*) csound->Malloc(csound, sizeof(CORFIL));
    ans->body = cs_strdup(csound, (char*)text);
    ans->len = strlen(text)+1;
    ans->p = 0;
    return ans;
}

void corfile_putc(CSOUND *csound, int c, CORFIL *f)
{
    f->body[f->p++] = c;
    if (UNLIKELY(f->p >= f->len)) {
      char *new = (char*) csound->ReAlloc(csound, f->body, f->len+=100);
      if (UNLIKELY(new==NULL)) {
        fprintf(stderr, Str("Out of Memory\n"));
        exit(7);
      }
      f->body = new;
    }
    f->body[f->p] = '\0';
}

void corfile_puts(CSOUND *csound, const char *s, CORFIL *f)
{
    const char *c;
    int n;
    /* skip and count the NUL chars to the end */
    for (n=0; f->p > 0 && f->body[f->p-1] == '\0'; n++, f->p--);
    /* append the string */
    for (c = s; *c != '\0'; c++) {
      f->body[f->p++] = *c;
      if (UNLIKELY(f->p >= f->len)) {
        char *new = (char*) csound->ReAlloc(csound, f->body, f->len+=100);
        if (UNLIKELY(new==NULL)) {
          fprintf(stderr, Str("Out of Memory\n"));
          exit(7);
        }
        f->body = new;
      }
    }
    if (n > 0) {
      /* put the extra NULL chars to the end */
      while (--n >= 0) {
        f->body[f->p++] = '\0';
        if (UNLIKELY(f->p >= f->len)) {
          char *new = (char*) csound->ReAlloc(csound, f->body, f->len+=100);
          if (UNLIKELY(new==NULL)) {
            fprintf(stderr, Str("Out of Memory\n"));
            exit(7);
          }
          f->body = new;
        }
      }
    }
    f->body[f->p] = '\0';
}

void corfile_flush(CSOUND *csound, CORFIL *f)
{
    char *new;
    f->len = strlen(f->body)+1;
    new = (char*)csound->ReAlloc(csound, f->body, f->len);
    if (UNLIKELY(new==NULL)) {
      fprintf(stderr, Str("Out of Memory\n"));
      exit(7);
    }
    f->body = new;
    f->p = 0;
}

#undef corfile_length
int corfile_length(CORFIL *f)
{
    return strlen(f->body);
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

int corfile_getc(CORFIL *f)
{
    int c = f->body[f->p];
    if (UNLIKELY(c=='\0')) return EOF;
    f->p++;
    return c;
}

char *corfile_fgets(char *buff, int len, CORFIL *f)
{
    int i;
    char *p = &(f->body[f->p]), *q;
    if (UNLIKELY(*p == '\0')) return NULL;
    q = strchr(p, '\n');
    i = (q-p);
    if (UNLIKELY(i>=len)) i = len-1;
    memcpy(buff, p, i);
    f->p += i;
    return buff;
}

#undef corfile_ungetc
void corfile_ungetc(CORFIL *f)
{
    --f->p;
}

MYFLT corfile_get_flt(CORFIL *f)
{
    int n = f->p;
    MYFLT ans;
    while (!isspace(f->body[++f->p]));
    ans = (MYFLT) atof(&f->body[n]);
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
int corfile_tell(CORFIL *f)
{
    return f->p;
}

#undef corfile_set
void corfile_set(CORFIL *f, int n)
{
    f->p = n;
}

void corfile_seek(CORFIL *f, int n, int dir)
{
    if (dir == SEEK_SET) f->p = n;
    else if (dir == SEEK_CUR) f->p += n;
    else if (dir == SEEK_END) f->p = strlen(f->body)-n;
    if (UNLIKELY(f->p > strlen(f->body))) {
      printf("INTERNAL ERROR: Corfile seek out of range\n");
      exit(1);
    }
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
void *fopen_path(CSOUND *csound, FILE **fp, const char *name,
                 const char *basename, char *env, int fromScore);
CORFIL *copy_to_corefile(CSOUND *csound, const char *fname,
                         const char *env, int fromScore)
{
    CORFIL *mm;
    FILE *ff;
    void *fd;
    int n;
    char buffer[1024];
    if (UNLIKELY(fname==NULL)) {
      csound->ErrorMsg(csound, Str("Null file name in copy_to_corefile"));
      csound->LongJmp(csound, 1);
    }
#ifdef HAVE_CURL
    if (strstr(fname,"://")) {
      /* **** Note the +2 is to skip spurous ./ inserted in a file name */
      return copy_url_corefile(csound, fname+2, fromScore);
    }
#endif
    fd = fopen_path(csound, &ff, (char *)fname, NULL, (char *)env, fromScore);
    if (UNLIKELY(ff==NULL)) return NULL;
    mm = corfile_create_w(csound);
    if (fromScore) corfile_putc(csound, '\n', mm);
    memset(buffer, '\0', 1024);
    while ((n = fread(buffer, 1, 1023, ff))) {
      /* Need to lose \r characters  here */
      /* while ((s = strchr(buffer, '\r'))) { */
      /*   int k = n - (s-buffer); */
      /*   memmove(s, s+1, k); */
      /*   n--; */
      /* } */
      corfile_puts(csound, buffer, mm);
      memset(buffer, '\0', 1024);
    }
    //#ifdef SCORE_PARSER
    if (fromScore) {
      corfile_puts(csound, "\ne\n#exit\n", mm);
    }
    //#endif
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    if (fromScore) corfile_flush(csound, mm);
    csoundFileClose(csound, fd);
    //if (fromScore) printf("Copy is >>%s<<\n", mm->body);
    return mm;
}

void corfile_preputs(CSOUND *csound, const char *s, CORFIL *f)
{
    char *body = f->body;
    f->body = (char*)csound->Malloc(csound, f->len=(strlen(body)+strlen(s)+1));
    f->p = f->len-1;
    strcpy(f->body, s); strcat(f->body, body);
    csound->Free(csound, body);
}

#ifdef HAVE_CURL

#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
  CSOUND* cs;
};


static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  CSOUND *csound = mem->cs;

  mem->memory = csound->ReAlloc(csound, mem->memory, mem->size + realsize + 1);
  if (UNLIKELY(mem->memory == NULL)) {
    /* out of memory! */
    printf(Str("not enough memory (realloc returned NULL)\n"));
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

CORFIL *copy_url_corefile(CSOUND *csound, const char *url, int fromScore)
{
    int n;
    CURL *curl = curl_easy_init();
    CORFIL *mm = corfile_create_w(csound);
    struct MemoryStruct chunk;

    chunk.memory = csound->Malloc(csound, 1);  /* will grown */
    chunk.size = 0;    /* no data at this point */
    chunk.cs = csound;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    n = curl_easy_perform(curl);
    if (UNLIKELY(n != CURLE_OK)) {
      csound->Die(csound, Str("curl_easy_perform() failed: %s\n"),
                  curl_easy_strerror(n));
      /* return NULL ? */
    }
    curl_easy_cleanup(curl);
    corfile_puts(csound, chunk.memory, mm);
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    corfile_putc(csound, '\0', mm);     /* For use in bison/flex */
    if (fromScore) corfile_flush(csound, mm);
    csound->Free(csound, chunk.memory);

    curl_global_cleanup();
    return mm;
}

#endif

#if 0
int main(void)
{
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    /* will grown as needed by the realloc above */
    chunk.memory = csound->Malloc(csound, 1);
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.example.com/");

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    else {
      /*
       * Now, our chunk.memory points to a memory block that is chunk.size
       * bytes big and contains the remote file.
       *
       * Do something nice with it!
       */

      printf("%lu bytes retrieved\n", (long)chunk.size);
    }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    if (chunk.memory)
      free(chunk.memory);

    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();

    return 0;
}
#endif

#ifdef JPFF
/* Start of directory of corfiles currently unused except experimental in CsFileC */
typedef struct dir {
  char       *name;
  CORFIL     *corfile;
  struct dir *next;
} CORDIR;

void add_corfile(CSOUND* csound, CORFIL *smpf, char *filename)
{
    CORDIR *entry = csound->Malloc(csound, sizeof(CORDIR));
    entry->name = cs_strdup(csound, filename);
    entry->corfile = smpf;
    entry->next = (CORDIR *)csound->directory;
    csound->directory = entry;
}
#endif
