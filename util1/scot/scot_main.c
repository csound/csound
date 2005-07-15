#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "csoundCore.h"

long natlong(long lval)     /* coerce a bigendian long into a natural long */
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

int main(int argc, char **argv)
{
  FILE *infile = 0,*outfile = 0;
  char *name = 0;

  init_getstring(argc, argv);
  csoundPreCompile(csoundCreate(NULL));
  if (argc==2)
  {
    if (!(infile=fopen(argv[1],"r")))
     csoundDie(&cenviron, Str("Can't open input file"));
    name=argv[1];
  }
  else if (argc==1)
  {
    infile=stdin;
    name="";
  }
  else csoundDie(&cenviron, "Usage:  scot <file>");
  if (!(outfile=fopen("score","w")))
   csoundDie(&cenviron, "Can't open output file \"score\"");
  scot(infile,outfile,name);
  fclose(infile);
  fclose(outfile);
  return(0);
}

