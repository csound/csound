/*  
    xmain.c

    Copyright (C) 1991 Barry Vercoe, John ffitch

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
#include "cs.h"                                    /*   XMAIN.C  */
#include <string.h>
#include <stdlib.h>

ENVIRON cenviron;

#ifndef POLL_EVENTS
int POLL_EVENTS(void)
{
    return (1);
}
#endif

void pvsys_release(void) {};

long named_instr_find (char *name)
{
    err_printf("WARNING: named instruments are not supported ");
    err_printf("by stand-alone utilities\n");
    err_printf("assuming insno = -1 for instr %s\n", name);
    return(-1L);
}

void dies(char *s, char *t)
{
    sprintf(errmsg,s,t);
    printf("%s\n",errmsg);
    exit(1);
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

void *memfiles = NULL;
void rlsmemfiles(void)
{
}

int main(int ac, char **av)         /* stdio stub for standalone extract */
                                    /*     first opens the control xfile */
{
    FILE *xfp;
    init_getstring(0, NULL);
    ac--;  av++;
    if (ac != 1) {
      fprintf(stderr,"usage: extract xfile <in >out\n");
      exit(1);
    }
    if ((xfp = fopen(*av,"r")) == NULL) {
      fprintf(stderr,"extract: can't open %s\n", *av);
      exit(1);
    }
    return(scxtract(stdin,stdout,xfp));
}

FILE *fopen_path(char *name, char *basename, char *env, char *mode)
{
    FILE *ff;
    char *p;
				/* First try to open name given */
    strcpy(name_full, name);
    if ((ff = fopen(name_full, mode))!=NULL) return ff;
				/* if that fails try in base directory */
    strcpy(name_full, basename);
#if defined(__MWERKS) || defined(SYMANTECS)
    p = strrchr(name_full, ':');
#else
    p = strrchr(name_full, '/');
    if (p==NULL) p = strrchr(name_full, '\\');
#endif
    if (p != NULL) {
      strcpy(p+1, name);
      if ((ff = fopen(name_full, mode))!=NULL) return ff;
				/* Of us env argument */
    }
    if ((p = getenv(env)))
#if defined(__MWERKS) || defined(SYMANTECS)
      sprintf(name_full, "%s:%s", p, name);
#else
      sprintf(name_full, "%s/%s", p, name);
#endif
    if ((ff = fopen(name_full, mode))!=NULL) return ff; 
    return NULL;		/* or give up */
}

void synterr(char *s)
{
  printf("error:  %s\n",s);
}


#ifndef CWIN
#include <stdarg.h>

void err_printf(char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
#endif
void csoundMessage0(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
