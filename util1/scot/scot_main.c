
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

extern int scot(FILE *inf, FILE *outf, char *fil);

int main(int argc, char **argv)
{
    FILE    *infile = 0, *outfile = 0;
    char    *name = 0;

    if (argc == 2) {
      if (!(infile = fopen(argv[1], "r"))) {
        fprintf(stderr, "Can't open input file");
        return -1;
      }
      name = argv[1];
    }
    else if (argc == 1) {
      infile = stdin;
      name = "";
    }
    else {
      fprintf(stderr, "Usage:  scot <file>");
      return -1;
    }
    if (!(outfile = fopen("score", "w"))) {
      fprintf(stderr, "Can't open output file \"score\"");
      return -1;
    }
    scot(infile, outfile, name);
    fclose(infile);
    fclose(outfile);

    return 0;
}

