/*
    het_import.c

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
/* ***************************************************************** */
/* ******** Program to import hetro files from tabular format. ***** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jul 14                                           */
/* ***************************************************************** */

#include "std_util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
/*#include "hetro.h"*/
#include "text.h"

#define END  32767

void het_import_usage(CSOUND *csound)
{
    csound->Message(csound, "%s", Str("Usage: het_import csvtext_file het_file\n"));
}

int16 getnum(FILE* inf, char *term)
{
    char buff[16];
    int32_t  cc;
    int32_t p = 0;
    while ((cc=getc(inf))!=',' && cc!='\n' && p<15) {
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

static int32_t het_import(CSOUND *csound, int32_t argc, char **argv)
{
    FILE    *infd;
    FILE    *outf;
    int32_t c;

    if (UNLIKELY(argc!= 3)) {
      het_import_usage(csound);
      return 1;
    }

    infd = fopen(argv[1], "r");
    if (UNLIKELY(infd == NULL)) {
      csound->Message(csound, Str("Cannot open input comma file %s\n"), argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "wb");
    if (UNLIKELY(outf == NULL)) {
      csound->Message(csound, Str("Cannot open output hetro file %s\n"), argv[2]);
      fclose(infd);
      return 1;
    }

    if ((c=getc(infd)) == 'H') {
      char buf[6];
      int32_t i;
      for (i=0; i<4; i++) buf[i]=(char)getc(infd);
      if (UNLIKELY(strncmp(buf, "ETRO", 4)!=0)) {
        csound->Message(csound, Str("Not an hetro anaysis file %s\n"), argv[1]);
        fclose(infd); fclose(outf);
        return 1;
      }
    }
    else ungetc(c, infd);
    for (;;) {
      int16 x;
      char  term;
      int16 end = END;
      x = getnum(infd, &term);
      if (term == '\0') break;
      if (UNLIKELY(1!=fwrite(&x, sizeof(int16), 1, outf)))
        fprintf(stderr, "Write failure\n");
      if (term == '\n')
        if (UNLIKELY(1!=fwrite(&end, sizeof(int16), 1, outf)))
          fprintf(stderr, "Write failure\n");
    }
    fclose(outf);
    fclose(infd);
    return 0;
}

/* module interface */

int32_t het_import_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "het_import", het_import);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "het_import",
                                      Str("translate text form to "
                                          "hetro analysis file"));
    }
    return retval;
}

