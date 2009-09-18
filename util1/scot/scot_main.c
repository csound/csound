
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

extern int scot(FILE *inf, FILE *outf, char *fil);

int main(int argc, char **argv)
{
    FILE    *infile = NULL, *outfile = NULL;
    char    *name = NULL,   *outname = "score";

    if (argc == 2 || argc == 3) {
      if (argc == 3) outname = argv[2];
      if (!(infile = fopen(argv[1], "r"))) {
        fprintf(stderr, "Can't open input file \"%s\"\n", argv[1]);
        return -1;
      }
      name = argv[1];
    }
    else if (argc == 1) {
      infile = stdin;
      name = "";
    }
    else {
      fprintf(stderr, "Usage:  scot [<infile> [<outfile>]]\n");
      return -1;
    }
    if (!(outfile = fopen(outname, "w"))) {
      fprintf(stderr, "Can't open output file \"%s\"\n", outname);
      return -1;
    }
    scot(infile, outfile, name);
    fclose(infile);
    fclose(outfile);

    return 0;
}

