
/* makecsd utility - written by Istvan Varga, Mar 2003 */
/* updated jpff Feb 2011 */

#define VERSION "2.0"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define MAXLICENCE (7)

static  char    *orcname = NULL, *sconame = NULL, *optname = NULL;
static  char    *midiname = NULL, *scorebin = NULL, *licence = NULL;
static  int     licencetype = -1;

static  int     nr_infiles = 0;
static  int     max_infiles = 0;
static  char    **infile_names = NULL;

static  int     output_is_stdout = 1;
static  char    *outflname = NULL;
static  FILE    *outfile = NULL;

static  int     tabstop_size = 0;

/* convert 6 bits of input and write to output file */
extern  int     encode_byte(FILE*, FILE*);
/* convert an entire input file */
extern  void    encode_file(char*, FILE*, int);
/* line width */
extern  int     maxlinepos;

static int setorchnam(char *s)
{
    char  *c;
    if (orcname != NULL) return 0;      /* orchestra name is already set */
    if (strlen(s) < 5) return 0;        /* name is too short */
    c = s + (int) strlen(s);
    /* check if extension is correct */
    --c; if (*c != 'C' && *c != 'c') return 0;
    --c; if (*c != 'R' && *c != 'r') return 0;
    --c; if (*c != 'O' && *c != 'o') return 0;
    --c; if (*c != '.') return 0;
    /* set orchestra name and report success */
    orcname = s;
    return 1;
}

static int setscornam(char *s)
{
    char  *c;
    if (sconame != NULL) return 0;      /* score name is already set */
    if (strlen(s) < 5) return 0;        /* name is too short */
    c = s + (int) strlen(s);
    /* check if extension is correct */
    --c; if (*c != 'O' && *c != 'o') return 0;
    --c; if (*c != 'C' && *c != 'c') return 0;
    --c; if (*c != 'S' && *c != 's') return 0;
    --c; if (*c != '.') return 0;
    /* set score name and report success */
    sconame = s;
    return 1;
}

/* read a single character from orc/sco input file, with check for */
/* end of file and Unix/MS-DOS/Mac format files */

static int mygetc(FILE *f, int *end_of_file)
{
    int c;

    if (*end_of_file) return (-1);      /* end of file */
    c = getc(f);
    if (c == '\r') {            /* deal with MS-DOS and Mac files */
      c = getc(f);
      if (c == EOF) {
        *end_of_file = 1;
        return '\n';
      }
      if (c != '\n')            /* CR-only: Mac format */
        ungetc(c, f);
      return '\n';
    }
    if (c == EOF) {
      *end_of_file = 1;                 /* end of file */
      return (-1);
    }
    return c;
}

/* write a line to output file, with tab->space conversion if requested */

static void write_line(FILE *outfl, char *buf, int n)
{
    int linepos = 0;
    while (n--) {
      if (tabstop_size && *buf == '\t') {       /* expand tabs to spaces */
        do {
          putc(' ', outfl);
        } while ((++linepos) % tabstop_size);
      }
      else {
        if (*buf == '\n') linepos = -1;     /* new line: reset line position */
        putc((int) *buf, outfl);
        linepos++;
      }
      buf++;
    }
}

static void convert_txt_file(char *inflname, FILE *outfl)
{
    FILE  *infile;
    char  linebuf[16384];
    int   linepos = 0, c, frstline = 1;
    int   is_eof;

    /* open file */
    infile = fopen(inflname, "rb");
    if (infile == NULL) {
      fprintf(stderr, "makecsd: error opening input file %s: %s\n",
                      inflname, strerror(errno));
      exit(-1);
    }
    is_eof = 0;
    /* read entire file and copy to output */
    while (!is_eof) {
      /* read next non-empty line to buffer */
      linepos = 0;
      do {
        c = mygetc(infile, &is_eof);
        if (is_eof || c == '\n') {
          /* end of line: remove any trailing white space */
          while (linepos && isblank(linebuf[linepos - 1]))
            linepos--;
          linebuf[linepos++] = '\n';
          if (is_eof ||                 /* end of file */
              (linepos > 1 && linebuf[linepos - 2] != '\n'))
            break;                      /* or a new non-empty line */
        }
        else
          linebuf[linepos++] = c;
      } while (1);
      if (is_eof) break;                /* end of file */
      /* at beginning of file: skip any leading empty lines */
      if (frstline) {
        int n = -1;
        frstline = 0;
        while (++n < linepos && linebuf[n] == '\n');
        /* if there is anything to print */
        if (n < linepos)
          write_line(outfl, &(linebuf[n]), linepos - n);
      }
      else  /* otherwise just print as it is */
        write_line(outfl, &(linebuf[0]), linepos);
    }
    /* at end of file: skip any trailing empty lines */
    while (linepos && linebuf[linepos - 1] == '\n')
      linepos--;
    linebuf[linepos++] = '\n';
    /* and print last line */
    write_line(outfl, &(linebuf[0]), linepos);
    /* close input file */
    fclose(infile); infile = NULL;
}

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
          fprintf(stderr, "makecsd: missing option for -w\n");
          exit(-1);
        }
        i++;
        maxlinepos = (int) atoi(argv[i]);
        if (maxlinepos < 20) maxlinepos = 20;
        if (maxlinepos > 200) maxlinepos = 200;
      }
      else if (!strcmp(argv[i], "-o")) {        /* output file name */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -o\n");
          exit(-1);
        }
        i++;
        outflname = argv[i];
        if (!strcmp(outflname, "-"))
          output_is_stdout = 1;
        else
          output_is_stdout = 0;
      }
      else if (!strcmp(argv[i], "-t")) {        /* tabstop size */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -t\n");
          exit(-1);
        }
        i++;
        tabstop_size = (int) atoi(argv[i]);
        if (tabstop_size < 0) tabstop_size = 0;
        if (tabstop_size > 24) tabstop_size = 24;
      }
      else if (!strcmp(argv[i], "-f")) {        /* options file name */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -f\n");
          exit(-1);
        }
        i++;
        optname = argv[i];
      }
      else if (!strcmp(argv[i], "-m")) {        /* options file name */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -m\n");
          exit(-1);
        }
        i++;
        midiname = argv[i];
      }
      else if (!strcmp(argv[i], "-b")) {        /* options file name */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -b\n");
          exit(-1);
        }
        i++;
        scorebin = argv[i];
      }
      else if (!strcmp(argv[i], "-L")) {        /* options file name */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -L\n");
          exit(-1);
        }
        i++;
        licence = argv[i];
        licencetype = 99;
      }
      else if (!strcmp(argv[i], "-l")) {        /* options file name */
        if (!(--j)) {
          fprintf(stderr, "makecsd: missing option for -l\n");
          exit(-1);
        }
        i++;
        licencetype = atoi(argv[i]);
        if (licencetype>MAXLICENCE || licencetype<0) licencetype = -1;
      }
      else {                                    /* input file name */
        if (*(argv[i]) == '-') {
          fprintf(stderr, "makecsd: invalid option: %s\n", argv[i]);
          exit(-1);
        }
        if (!(setorchnam(argv[i]) || setscornam(argv[i]))) {
          /* if input is neither an orchestra, nor a score file, */
          /* encode it as Base64 */
          if (nr_infiles >= max_infiles) {
            /* extend number of input files */
            max_infiles = max_infiles + (max_infiles >> 2) + 16;
            infile_names = (char**) realloc(infile_names,
                                            sizeof(char*) * max_infiles);
            if (infile_names == NULL) {
              fprintf(stderr, "makecsd: not enough memory\n");
              exit(-1);
            }
          }
          infile_names[nr_infiles] = argv[i];
          nr_infiles++;
        }
      }
    }
    if (!nr_infiles && (orcname == NULL && sconame == NULL)) {
      /* print usage */
      fprintf(stderr, "makecsd: no input files\n");
      fprintf(stderr,
              "usage: makecsd [OPTIONS ... ] infile1 [ infile2 [ ... ]]\n");
      fprintf(stderr, "options:\n");
      fprintf(stderr,
              "    -t <n>      expand tabs to spaces using tabstop size ");
      fprintf(stderr, "n (default: disabled)\n");
      fprintf(stderr,
              "    -w <n>      set Base64 line width to n (default: 72)\n");
      fprintf(stderr,
              "    -f <fname>  read CSD options from file 'fname'\n");
      fprintf(stderr,
              "    -o <fname>  output file name (default: stdout)\n");
      /* **** NEW OPTIONS **** */
      //      fprintf(stderr,
      //              "    -v[ba] x.y  use version (before/after) x.y\n");
      fprintf(stderr,
              "    -l int      licence type (0-1)\n");
      fprintf(stderr,
              "    -L fname    licence file\n");
      fprintf(stderr,
              "    -b <pname>  program to process score'\n");
      fprintf(stderr,
              "    -m <fname>  MIDI file name\n");
      exit(-1);
    }
    /* open output file */
    if (!output_is_stdout) {
      outfile = fopen(outflname, "w");
      if (outfile == NULL) {
        fprintf(stderr, "makecsd: error opening output file %s: %s\n",
                        outflname, strerror(errno));
        exit(-1);
      }
    }
    else
      outfile = stdout;
    /* create CSD file */
    fprintf(outfile, "<CsoundSynthesizer>\n");
    fprintf(outfile, ";;; this CSD file generated by makecsd v" VERSION "\n");
    fprintf(outfile, "<CsOptions>\n");
    fprintf(outfile, ";;; set command line options here\n");
    /* CSD options */
    for (i = 0; i < nr_infiles; i++) {
      char *s = infile_names[i], *s0;
      j = (int) strlen(s);
      if (j < 4) continue;
      /* check for MIDI file */
      if (s[j - 1] != 'D' && s[j - 1] != 'd') continue;
      if (s[j - 2] != 'I' && s[j - 2] != 'i') continue;
      if (s[j - 3] != 'M' && s[j - 3] != 'm') continue;
      if (s[j - 4] != '.') continue;
      /* add to options if found */
      /* strip any leading components from file name */
      s0 = s; s = s0 + ((int) strlen(s0) - 1);
      while (s >= s0 && *s != '/' && *s != '\\' && *s != ':') s--;
      s++;
      fprintf(outfile, "-F %s\n", s);
      break;
    }
    if (optname != NULL) {
      /* copy options file if specified */
      convert_txt_file(optname, outfile);
      /* hack: remove blank line from end of options */
      fflush(outfile);
      if (fseek(outfile, -1L, SEEK_END)<0) {
        fprintf(stderr, "makecsd: seek failed\n");
        exit(1);
      }
    }
    else if (i >= nr_infiles) {
      fprintf(outfile, "\n");   /* put blank line if there are no options */
    }
    fprintf(outfile, "</CsOptions>\n");
    if (licencetype == 99) {    /* Licence file */
      fprintf(outfile, "<CsLicence>\n");
      convert_txt_file(licence, outfile);
      fprintf(outfile, "</CsLicence>\n");
    }
    else if (licencetype < MAXLICENCE) { /* precanned licence */
      char buff[32];
      time_t tt = time(NULL);
      strftime(buff, 32, "<CsLicence>\nCopyright %Y: ", gmtime(&tt));
      fprintf(outfile, buff);
      switch (licencetype) {
      default:
      case 0:
        fprintf(outfile, "All Rights Reserved\n");
        break;
      case 1:
        fprintf(outfile, "Creative Commons "
                "Attribution-NonCommercial-NoDerivatives\n"
                "CC BY-NC-ND\n");
        break;
      case 2:
        fprintf(outfile, "Creative Commons "
                "Attribution-NonCommercial-ShareAlike\n"
                "CC BY-NC-SA\n");
        break;
      case 3:
        fprintf(outfile, "Creative Commons "
                "Attribution-NonCommercial\n"
                "CC BY-NC\n");
         break;
     case 4:
        fprintf(outfile, "Creative Commons "
                "Attribution-NoDerivatives\n"
                "CC BY-ND\n");
        break;
      case 5:
        fprintf(outfile, "Creative Commons "
                "Attribution-ShareAlike\n"
                "CC BY-SA\n");
        break;
      case 6:
        fprintf(outfile, "Creative Commons "
                "Attribution\n"
                "CC BY\n");
        break;
      case 7:
        fprintf(outfile, "Licenced under BSD\n");
        break;
      }
      fprintf(outfile, "</CsLicence>\n");
    }
    /* orchestra */
    if (orcname != NULL) {
      fprintf(outfile, "<CsInstruments>\n");
      convert_txt_file(orcname, outfile);
      fprintf(outfile, "</CsInstruments>\n");
    }
    else {
      fprintf(stderr, "makecsd: warning: no orchestra file\n");
    }
    /* score */
    if (sconame != NULL) {
      if (scorebin)
        fprintf(outfile, "<CsScore bin=\"%s\">\n", scorebin);
      else
        fprintf(outfile, "<CsScore>\n");
      convert_txt_file(sconame, outfile);
      fprintf(outfile, "</CsScore>\n");
    }
    else {
      fprintf(stderr, "makecsd: warning: no score file\n");
    }
    if (midiname != NULL) {
      fprintf(outfile, "<CsMidifileB>\n");
      encode_file(midiname, outfile, 1);
      fprintf(outfile, "</CsMidifileB>\n");
    }
    /* Base64 encode any remaining input files */
    for (i = 0; i < nr_infiles; i++)
      encode_file(infile_names[i], outfile, 0);
    /* end of CSD file */
    fprintf(outfile, "</CsoundSynthesizer>\n\n");
    /* close output file */
    if (!output_is_stdout) {
      fflush(outfile);
      fclose(outfile);
    }
    free(infile_names);
    return 0;
}

