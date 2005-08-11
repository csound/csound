/*
    het_import.c

    Copyright (C) 1995 John ffitch

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* ***************************************************************** */
/* ******** Program to import hetro files from tabular format. ***** */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jul 14                                           */
/* ***************************************************************** */

#include "csdl.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
/*#include "hetro.h"*/
#include "text.h"

#define END  32767

void het_import_usage(ENVIRON *csound)
{
    csound->Message(csound, Str("Usage: het_import commafile hetfile\n"));
}

short getnum(FILE* inf, char *term)
{
    char buff[100];
    int  cc;
    int p = 0;
    while ((cc=getc(inf))!=',' && cc!='\n') {
        if (cc == EOF) {
            *term = '\0';
            return 0;
        }
        buff[p++] = cc;
    }
    buff[p]='\0';
    *term = cc;
    return (short)atoi(buff);
}

static int het_import(ENVIRON *csound, int argc, char **argv)
{
    FILE *infd;
    FILE *outf;

    if (argc!= 3) {
      het_import_usage(csound);
      return 1;
    }

    infd = fopen(argv[1], "r");
    if (infd == NULL) {
      csound->Message(csound, Str("Cannot open input comma file%s\n"), argv[1]);
      return 1;
    }
    outf = fopen(argv[2], "wb");
    if (outf == NULL) {
      csound->Message(csound, Str("Cannot open output hetro file %s\n"), argv[2]);
      exit(1);
    }

    for (;;) {
      short x;
      char term;
      short end = END;
      x = getnum(infd, &term);
      if (term == '\0') break;
      fwrite(&x, 1, sizeof(short), outf);
      if (term == '\n')  fwrite(&end, 1, sizeof(short), outf);
    }
    fclose(outf);
    fclose(infd);
    return 0;
}

/* module interface */

PUBLIC int csoundModuleCreate(ENVIRON *csound)
{
    int retval = csound->AddUtility(csound, "het_import", het_import);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "het_import",
                    "translate text form to hetro analysis file");
    }
    return retval;
}

