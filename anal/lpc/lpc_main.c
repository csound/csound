/*  
    lpc_main.c for lpc

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
#include <stdio.h>
#include "cs.h"
#include "ustub.h"

MYFLT e0dbfs = DFLT_DBFS;
#ifdef HAVE_FLTK
int fltk_abort = 0;
#endif

#ifdef WINDOWS
int  Graphable(void){return Graphable_();}
void MakeGraph(WINDAT *x, char *y){MakeGraph_(x,y);}
void MakeXYin(XYINDAT *x, MYFLT y, MYFLT z){MakeXYin_(x,y,z);}
void DrawGraph(WINDAT *x){DrawGraph_(x);}
void ReadXYin(XYINDAT *x){ReadXYin_(x);}
void KillGraph(WINDAT *x){KillGraph_(x);}
void KillXYin(XYINDAT *x){KillXYin_(x);}
int  ExitGraph(void){ return ExitGraph_();}
#endif

int main(int argc, char **argv)
{
    scorename = "LPC";
    orchname = "LPC";
    init_getstring(argc,argv);
    return lpanal(argc,argv);
}

#ifndef CWIN
#include <stdarg.h>

void dribble_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vprintf(fmt, a);
    va_start(a, fmt);
    va_end(a);
    if (dribble != NULL) {
      va_start(a, fmt);
      vfprintf(dribble, fmt, a);
      va_end(a);
    }
}

void err_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    if (dribble != NULL) {
      va_start(a, fmt);
      vfprintf(dribble, fmt, a);
      va_end(a);
    }
}
#endif
void csoundMessage0(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
