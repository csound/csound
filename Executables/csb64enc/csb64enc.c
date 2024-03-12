
/* Base64 encoder utility - written by Istvan Varga, Jan 2003 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static  int     nr_infiles = 0;
static  int     max_infiles = 0;
static  char    **infile_names = NULL;
static  int     style      = 0;
static  int     output_is_stdout = 1;
static  char    *outflname = NULL;
static  FILE    *outfile = NULL;

/* convert 6 bits of input and write to output file */
extern  int     encode_byte(FILE*, FILE*);
/* convert an entire input file */
extern  void    encode_file(char*, FILE*, int);
/* line width */
extern  int     maxlinepos;

int main(int argc, char **argv)
{
    int i, j;

    /* parse command line options */
    i = 0;
    j = argc;
    while (--j) {
      i++;
      if (!strcmp(argv[i], "-w")) {             /* line width */
        if (!(--j)) {
          fprintf(stderr, "csb64enc: missing option for -w\n");
          exit(-1);
        }
        i++;
        maxlinepos = (int) atoi(argv[i]);
        if (maxlinepos < 20) maxlinepos = 20;
        if (maxlinepos > 200) maxlinepos = 200;
      }
      else if (!strcmp(argv[i], "-o")) {        /* output file name */
        if (!(--j)) {
          fprintf(stderr, "csb64enc: missing option for -o\n");
          exit(-1);
        }
        i++;
        outflname = argv[i];
        if (!strcmp(outflname, "-"))
          output_is_stdout = 1;
        else
          output_is_stdout = 0;
      }
      else if (!strcmp(argv[i], "-s")) {
        if (!(--j)) {
          fprintf(stderr, "csb64enc: missing option for -s\n");
          exit(-1);
        }
        i++;
        style = atoi(argv[i]);
      }
      else {                                    /* input file name */
        if (*(argv[i]) == '-') {
          fprintf(stderr, "csb64enc: invalid option: %s\n", argv[i]);
          exit(-1);
        }
        if (nr_infiles >= max_infiles) {
          /* extend number of input files */
          max_infiles = max_infiles + (max_infiles >> 2) + 16;
          infile_names = (char**) realloc(infile_names,
                                          sizeof(char*) * max_infiles);
          if (infile_names == NULL) {
            fprintf(stderr, "csb64enc: not enough memory\n");
            exit(-1);
          }
        }
        infile_names[nr_infiles] = argv[i];
        nr_infiles++;
      }
    }
    if (!nr_infiles) {
      /* print usage */
      fprintf(stderr, "csb64enc: no input files\n");
      fprintf(stderr,
              "usage: csb64enc [OPTIONS ... ] infile1 [ infile2 [ ... ]]\n");
      fprintf(stderr, "options:\n");
      fprintf(stderr,
              "    -w <n>      set line width to n (default: 72)\n");
      fprintf(stderr,
              "    -o <fname>  output file name (default: stdout)\n");
      fprintf(stderr,
              "    -s <n>      output style to 0 (CsFileB default)\n"
              "                or 2 (CsFileC) or 1 (MIDI)\n");
      exit(-1);
    }
    /* open output file */
    if (!output_is_stdout) {
      outfile = fopen(outflname, "w");
      if (outfile == NULL) {
        fprintf(stderr, "csb64enc: error opening output file %s: %s\n",
                        outflname, strerror(errno));
        exit(-1);
      }
    }
    else
      outfile = stdout;
    /* encode all input files */
    for (i = 0; i < nr_infiles; i++)
      encode_file(infile_names[i], outfile, style);
    /* close output file */
    if (!output_is_stdout) {
      fflush(outfile);
      fclose(outfile);
    }
    free(infile_names);
    return 0;
}

