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
int  ExitGraph(void){int ExitGraph_();}
#endif

void pvsys_release(void) {};

int main(int argc, char **argv)
{
    scorename = "Cvanal";
    orchname = "Cvanal";
    init_getstring(argc,argv);
    return cvanal(argc,argv);
}

#ifndef CWIN
#include <stdarg.h>
void dribble_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vprintf(fmt, a);
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
    vfprintf(stderr, format, args);
    va_end(args);
}
