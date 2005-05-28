
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static  uint8_t file_buf[4194304];
static  uint8_t line_buf[65536];
static  int     empty_line_cnt = 0;

static int copy_line(int *pos, int len)
{
    int retval = 0;

    if (len < 1 || line_buf[len - 1] != '\n') {
      line_buf[len++] = '\n';
      retval = 1;
    }
    if (len > 1 && line_buf[len - 2] == ' ') {
      do {
        len--;
      } while (len > 1 && line_buf[len - 2] == ' ');
      line_buf[len - 1] = '\n';
      retval = 1;
    }
    if (len == 1) {
      if (++empty_line_cnt > 1)
        return 1;
    }
    else
      empty_line_cnt = 0;
    memcpy(&(file_buf[*pos]), &(line_buf[0]), len);
    *pos += len;
    return retval;
}

int main(int argc, char **argv)
{
    FILE    *f;
    int     flen, pos = 0, changed = 0, linepos = 0;
    int     c;

    if (argc < 2)
      return -1;
    f = fopen(argv[1], "rb");
    if (f == NULL)
      return -1;
    fseek(f, 0L, SEEK_END);
    flen = (int) ftell(f);
    if (flen < 1 || flen >= 4194304) {
      fclose(f);
      return -1;
    }
    fseek(f, 0L, SEEK_SET);
    while ((c = getc(f)) != EOF) {
      if (c == '\r') {
        changed = 1;
        if ((c = getc(f)) == EOF) {
          line_buf[linepos++] = '\n';
          break;
        }
        if (c != '\n') {
          ungetc(c, f);
          c = '\n';
        }
      }
      if (c == '\t') {
        changed = 1;
        do {
          line_buf[linepos++] = ' ';
        } while (linepos & 7);
        continue;
      }
      if (c == '\n') {
        line_buf[linepos++] = (uint8_t) c;
        changed |= copy_line(&pos, linepos);
        linepos = 0;
        continue;
      }
      line_buf[linepos++] = (uint8_t) c;
    }
    if (linepos) {
      if ((int) line_buf[linepos - 1] != '\n')
        changed = 1, line_buf[linepos++] = '\n';
      changed |= copy_line(&pos, linepos);
    }
    if (changed) {
      printf("%s\n", argv[1]);
      f = fopen(argv[1], "wb");
      if (f == NULL)
        return -1;
      fwrite(&(file_buf[0]), 1, pos, f);
      fflush(f);
      fclose(f);
    }
    return 0;
}

