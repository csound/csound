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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"     /*                              CORFILES.C      */
#include <string.h>
#include <stdio.h>

extern int csoundFileClose(CSOUND*, void*);

CORFIL *corfile_create_w(void)
{
    CORFIL *ans = (CORFIL*)malloc(sizeof(CORFIL));
    ans->body = (char*)calloc(100,1);
    ans->len = 100;
    ans->p = 0;
    return ans;
}

CORFIL *corfile_create_r(const char *text)
{
    char *strdup(const char *);
    CORFIL *ans = (CORFIL*)malloc(sizeof(CORFIL));
    ans->body = strdup(text);
    ans->len = strlen(text)+1;
    ans->p = 0;
    return ans;
}

void corfile_putc(int c, CORFIL *f)
{
    if (f->p+1 >= f->len)
      f->body = (char*) realloc(f->body, f->len+=100);
    f->body[f->p] = c;
    f->body[++f->p] = '\0';
}

void corfile_puts(char *s, CORFIL *f)
{
    int slen = strlen(s);
    while (f->p+slen+1>=f->len)
      f->body = (char*) realloc(f->body, f->len+=100);
    strcat(f->body, s);
    f->p += slen;
}

void corfile_flush(CORFIL *f)
{
    f->len = strlen(f->body)+1;
    f->body = (char*)realloc(f->body, f->len);
    f->p = 0;
}

#undef corfile_length
int corfile_length(CORFIL *f)
{
    return strlen(f->body);
}

void corfile_rm(CORFIL **ff)
{
    CORFIL *f = *ff;
    if (f!=NULL) {
      free(f->body);
      free(f);
      *ff = NULL;
    }
}

int corfile_getc(CORFIL *f)
{
    int c = f->body[f->p];
    if (c=='\0') return EOF;
    f->p++;
    return c;
}

#undef corfile_ungetc
void corfile_ungetc(CORFIL *f)
{
    --f->p;
}

MYFLT corfile_get_flt(CORFIL *f)
{
    int n;
    MYFLT ans;
#ifdef USE_DOUBLE
    sscanf(&f->body[f->p], "%lf%n", &ans, &n);
#else
    sscanf(&f->body[f->p], "%f%n", &ans, &n);
#endif
    f->p += n;
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
    if (f->p < 0 || f->p > strlen(f->body)) {
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
void *fopen_path(CSOUND *csound, FILE **fp, char *name, char *basename,
                 char *env, int fromScore);
CORFIL *copy_to_corefile(CSOUND *csound, char *fname, char *env, int fromScore)
{
    CORFIL *mm;
    FILE *ff;
    void *fd;
    int n;
    char buffer[1024];

    fd = fopen_path(csound, &ff, fname, NULL, env, fromScore);
    if (ff==NULL) return NULL;
    mm = corfile_create_w();
    memset(buffer, '\0', 1024);
    while ((n = fread(buffer, 1, 1023, ff))) {
      corfile_puts(buffer, mm);
      memset(buffer, '\0', 1024);
    }
    corfile_putc('\0', mm);     /* For use in bison/flex */
    corfile_putc('\0', mm);     /* For use in bison/flex */
    corfile_flush(mm);
    csoundFileClose(csound, fd);
    return mm;
}

