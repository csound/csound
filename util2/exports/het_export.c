/*  
    het_export.c

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
/* ******** Program to export hetro files in tabular format. ******* */
/* ***************************************************************** */

/* ***************************************************************** */
/* John ffitch 1995 Jun 19                                           */
/* ***************************************************************** */
     
#include <stdio.h>
#include <stdarg.h>
#include "cs.h"

MEMFIL *mfp;
OPARMS  O;			/* Not actually used */
GLOBALS cglob;

#define END  32767

void usage(void);

int initerror(char *s)
{
    printf(Str(X_300,"INIT ERROR: %s\n"), s);
    exit(1);
}

int main(int argc, char **argv)
{
    MEMFIL *inf;
    FILE *outf;
    short *adp;
    short *endata;
    int cc = 0;

    init_getstring(0, NULL);
    if (argc!= 3)
	usage();
    inf = ldmemfile(argv[1]);
    if (inf == NULL) {
      fprintf(stderr, Str(X_214,"Cannot open input file %s\n"), argv[1]);
	exit(1);
    }
    outf = fopen(argv[2], "w");
    if (outf == NULL) {
      fprintf(stderr, Str(X_215,"Cannot open output file %s\n"), argv[2]);
	exit(1);
    }
    adp = (short *) inf->beginp;
    endata = (short *) inf->endp;
    cc = 0;
    for (; adp<endata; adp++) {
	if (*adp == END) fputc('\n',outf), cc = 0;
	else fprintf(outf, "%s%hd", (cc ? ",":""), *adp), cc = 1;
    }
    fclose(outf);
}

void usage(void)
{
    exit(1);
}

void err_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}

long natlong(long lval)             /* coerce a bigendian long into a natural long */
{
    unsigned char benchar[4];
    unsigned char *p = benchar;
    long natlong;

    *(long *)benchar = lval;
    natlong = *p++;
    natlong <<= 8;
    natlong |= *p++;
    natlong <<= 8;
    natlong |= *p++;
    natlong <<= 8;
    natlong |= *p;
    return(natlong);
}

