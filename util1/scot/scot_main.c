#include <stdio.h>
#include <stdarg.h>
#include "text.h"

void die(char *);

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

FILE *dribble = NULL;
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
      va_start(a, fmt); /* gab */
      vfprintf(dribble, fmt, a);
      va_end(a);
    }
}

int main(int argc, char **argv)
{
  FILE *infile,*outfile;
  char *name;

  init_getstring(argc, argv);
  if (argc==2)
  {
    if (!(infile=fopen(argv[1],"r")))
     die(Str(X_214, "Can't open input file"));
    name=argv[1];
  }
  else if (argc==1)
  {
    infile=stdin;
    name="";
  }
  else die("Usage:  scot <file>");
  if (!(outfile=fopen("score","w")))
   die("Can't open output file \"score\"");
  scot(infile,outfile,name);
  fclose(infile);
  fclose(outfile);
  exit(0);
}


void die(char *s)
{
  printf("scot: %s\n",s);
  exit(1);
}
