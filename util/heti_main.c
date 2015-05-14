/*
    het_import_sa.c

    Copyright (C) 1995 John ffitch
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
/* ***************************************************************** */
/* ******** Program to import hetro files from tabular format. ***** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jul 14                                           */
/* This is the version that does not need all of csound as well      */
/* ***************************************************************** */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#define END  32767

#include <stdint.h>
#if defined(HAVE_GCC3)
#  define UNLIKELY(x)   __builtin_expect(!!(x),0)
#else
#  define UNLIKELY(x)   x
#endif

typedef int_least32_t int32;
typedef int_least16_t int16;
typedef uint_least32_t uint32;
typedef uint_least16_t uint16;

void het_import_usage(void)
{
    printf("Usage: het_import cstext_file het_file\n");
}

int16 getnum(FILE* inf, char *term)
{
    char buff[100];
    int  cc;
    int p = 0;
    while ((cc=getc(inf))!=',' && cc!='\n' && p<99) {
      if (UNLIKELY(cc == EOF)) {
        *term = '\0';
        return 0;
      }
      buff[p++] = cc;
    }
    buff[p]='\0';
    *term = cc;
    return (int16)atoi(buff);
}

int main(int argc, char **argv)
{
    FILE *infd;
    FILE *outf;

    if (UNLIKELY(argc!= 3)) {
      het_import_usage();
      return 1;
    }

    infd = fopen(argv[1], "r");
    if (UNLIKELY(infd == NULL)) {
      printf("Cannot open input comma file%s\n", argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "wb");
    if (UNLIKELY(outf == NULL)) {
      printf("Cannot open output hetro file %s\n", argv[2]);
      fclose(infd);
      return 1;
    }

    for (;;) {
      int16 x;
      char term;
      int16 end = END;
      x = getnum(infd, &term);
      if (UNLIKELY(term == '\0')) break;
      if (UNLIKELY(fwrite(&x, sizeof(int16), 1, outf)!=1)) exit(1);
      if (term == '\n')
        if (UNLIKELY(fwrite(&end, sizeof(int16), 1, outf)!=1)) exit(1);
    }
    fclose(outf);
    fclose(infd);
    return 0;
}

