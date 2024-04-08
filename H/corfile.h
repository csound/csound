/*
    corfiles.h:

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

#ifndef __corfil

#define __corfil

CORFIL *corfile_create_w(CSOUND*);
CORFIL *corfile_create_r(CSOUND*,const char *text);
void corfile_putc(CSOUND*,int32_t c, CORFIL *f);
void corfile_puts(CSOUND*,const char *s, CORFIL *f);
void corfile_flush(CSOUND*,CORFIL *f);
void corfile_rm(CSOUND*,CORFIL **ff);
int32_t corfile_getc(CORFIL *f);
void corfile_ungetc(CORFIL *f);
char *corfile_fgets(char *b, int32_t len, CORFIL *f);
#define corfile_ungetc(f)  (--f->p)
MYFLT corfile_get_flt(CORFIL *f);
void corfile_reset(CORFIL *f);
#define corfile_reset(f) (f->body[f->p=0]='\0')
void corfile_rewind(CORFIL *f);
#define corfile_rewind(f) (f->p=0)
int32_t corfile_tell(CORFIL *f);
#define corfile_tell(f) (f->p)
char *corfile_body(CORFIL *f);
#define corfile_body(f) (f->body)
char *corfile_current(CORFIL *f);
#define corfile_current(f) (f->body+f->p)
CORFIL *copy_to_corefile(CSOUND *, const char *, const char *, int32_t);
CORFIL *copy_url_corefile(CSOUND *, const char *, int32_t);
int32_t corfile_length(CORFIL *f);
#define corfile_length(f) (strlen(f->body))
void corfile_set(CORFIL *f, int32_t n);
#define corfile_set(f,n) (f->p = n)
void corfile_seek(CORFIL *f, int32_t n, int32_t dir);
void corfile_preputs(CSOUND *csound, const char *s, CORFIL *f);
void add_corfile(CSOUND* csound, CORFIL *smpf, char *filename);
#endif
