/*  
    fprint.c:

    Copyright (C) 1999 Gabriel Maldonado, John ffitch

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

#include "fout.h"

int fprintf_k(FPRINTF *p)
{
    char 	string[8192];
    sprints(string, p->txtstring, p->argums, p->INOCOUNT-2);
    fprintf(p->fp, string);
    return OK;
}

/* i-rate fprints */
int fprintf_i(FPRINTF *p)
{
    char 	string[8192];
    fprintf_set(p);
    sprints(string, p->txtstring, p->argums, p->INOCOUNT-2);
    fprintf(p->fp, string);
    return OK;
}

