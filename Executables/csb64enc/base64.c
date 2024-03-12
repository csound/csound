
/* Base64 encoder utility - written by Istvan Varga, Jan 2003 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static const char *encode_table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static  int     linepos = 0;
        int     maxlinepos = 72;                /* max line width */
static  int     bitcnt = 0, inval = 0;
static  FILE    *infile = NULL;

/* convert 6 bits of input and write to output file */

int encode_byte(FILE *infl, FILE *outfl)
{
    int c;

    if (bitcnt < 0) return 0;   /* end of file */
    if (bitcnt < 6) {                   /* read next byte from input file */
      c = getc(infl);
      if (c != EOF) {
        bitcnt += 8;
        inval |= ((c & 0xFF) << (24 - bitcnt));
      }
    }
    if (bitcnt == 0) return 0;  /* end of file */
    if (!(linepos % maxlinepos))
      putc('\n', outfl);                /* wrap line */
    linepos++;
    /* convert left 6 bits */
    putc((int) encode_table[((inval & 0x00FC0000) >> 18)], outfl);
    inval = (inval << 6) & 0x00FFFFC0;
    bitcnt -= 6;
    return 1;
}

/* convert an entire input file */

void encode_file(char *inflname, FILE *outfl, int style)
{
    char  *s, *s0;

    s0 = inflname;
    s = s0 + (int) strlen(s0);
    /* remove any leading components from filename */
    do {
      s--;
    } while (s >= s0 && *s != '/' && *s != '\\' && *s != ':');
    s++;
    if (strlen(s) < 1) {
      fprintf(stderr, "csb64enc: \"%s\": invalid input file name\n", s0);
      exit(-1);
    }
    /* open file */
    infile = fopen(s0, "rb");
    if (infile == NULL) {
      fprintf(stderr, "csb64enc: error opening input file %s: %s\n",
                      s0, strerror(errno));
      exit(-1);
    }
    /* create new CSD tag and encode file */
    if (style==1)
      fprintf(outfl, "<CsMidifileB>");
    else if (style==2)
      fprintf(outfl, "<CsFileC filename=\"%s\">", s);
    else
      fprintf(outfl, "<CsFileB filename=\"%s\">", s);
    linepos = bitcnt = inval = 0;
    while (encode_byte(infile, outfl));
    if (style)
      fprintf(outfl, "\b</CsMidifileB>\n");
    else
      fprintf(outfl, "\n</CsFileB>\n");
    /* close file */
    fclose(infile); infile = NULL;
}
