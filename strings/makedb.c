/*
    makedb.c:

    Copyright (C) 1999 John ffitch
    Jan 27 2005: replaced with new implementation by Istvan Varga

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#elif HAVE_STRINGS_H
# include <strings.h>
#endif
#include <ctype.h>
#include "text.h"

static int read_line(FILE *f, unsigned char *buf)
{
    int i, j, c;

    i = j = 0;
    do {
      c = fgetc(f);
      if (c == EOF || c == '\n')
        break;
      if (c == '"' && !(i > 0 && buf[i - 1] == (unsigned char) '\\'))
        j = 1 - j;
      else if (j) {
        buf[i] = (unsigned char) c;
        if (i > 0 && buf[i - 1] == (unsigned char) '\\') {
          if (c == 't')
            buf[--i] = '\t';
          else if (c == 'r')
            buf[--i] = '\r';
          else if (c == 'n')
            buf[--i] = '\n';
          else if (c == 'a')
            buf[--i] = '\a';
          else if (c == 'b')
            buf[--i] = '\b';
          else if (c == '"')
            buf[--i] = '"';
          else if (c == '\\')
            buf[--i] = '\\';
        }
        i++;
      }
    } while (1);
    buf[i] = '\0';
    return (c == EOF ? -1 : 0);
}

/* File format of string database files:                                    */
/*                                                                          */
/* Bytes 0 to 3:    magic number (the string "cStr")                        */
/* Bytes 4, 5:      file version (two bytes, big-endian, should be 0x1000)  */
/* Bytes 6, 7:      language code (two bytes, big-endian; see n_getstr.h)   */
/* Bytes 8 to 11:   number of strings (four bytes, big-endian, must be > 0) */
/* From byte 12:                                                            */
/*   list of strings, in the following format:                              */
/*     0x81     (1 byte)                                                    */
/*     original string, terminated with '\0' (maximum length is 16383)      */
/*     0x82     (1 byte)                                                    */
/*     translated string, terminated with '\0' (maximum length is 16383)    */
/*   there should be as many string pairs as defined by bytes 8 to 11.      */

int main(int argc, char **argv)
{
    FILE *db;
    FILE *base;
    FILE *trans;
    int  langcode, i, nmsgs;
    char buf[768];
    unsigned char buf2[768];

    /* find out language, and open files */
    base = fopen("strings/english-strings", "rb");
    if (argc < 3) {
      langcode = CSLANGUAGE_ENGLISH_UK;
      strcpy(&(buf[0]), "csound");
    }
    else {
      for (i = 0; (i < strlen(argv[2]) && i < 251); i++)
        buf[i] = (argv[2][i] >= 'A' && argv[2][i] <= 'Z' ?
                  argv[2][i] - 'A' + 'a' : argv[2][i]);
      buf[i] = '\0';
      if (strncmp(&(buf[0]), "american", 7) == 0 ||
          strncmp(&(buf[0]), "us", 2) == 0)
        langcode = CSLANGUAGE_ENGLISH_US;
      else if (strncmp(&(buf[0]), "french", 6) == 0)
        langcode = CSLANGUAGE_FRENCH;
      else if (strncmp(&(buf[0]), "spanish", 7) == 0)
        langcode = CSLANGUAGE_SPANISH;
      else if (strncmp(&(buf[0]), "german", 6) == 0)
        langcode = CSLANGUAGE_GERMAN;
      else if (strncmp(&(buf[0]), "italian", 7) == 0)
        langcode = CSLANGUAGE_ITALIAN;
      else
        langcode = CSLANGUAGE_ENGLISH_UK;
      strcpy(&(buf[0]), argv[2]);
    }
    strcat(&(buf[0]), ".xmg");
    db = fopen(&(buf[0]), "wb");
#if 0
    switch (langcode) {
      case CSLANGUAGE_ENGLISH_US:
        strcpy(&(buf[0]), "strings/all_strings");     break;
      case CSLANGUAGE_FRENCH:
        strcpy(&(buf[0]), "strings/french-strings");  break;
      case CSLANGUAGE_SPANISH:
        strcpy(&(buf[0]), "strings/spanish-strings"); break;
      case CSLANGUAGE_GERMAN:
        strcpy(&(buf[0]), "strings/german-strings");  break;
      case CSLANGUAGE_ITALIAN:
        strcpy(&(buf[0]), "strings/italian-strings"); break;
      default:
        strcpy(&(buf[0]), "strings/english-strings");
    }
#endif
    if (argc > 1)
      trans = fopen(argv[1], "rb");
    else
      trans = fopen("strings/english-strings", "rb");
    if (base == NULL || trans == NULL || db == NULL) {
      fprintf(stderr, "makedb: error opening file\n");
      return -1;
    }
    /* write file header */
    fputc('c', db); fputc('S', db); fputc('t', db); fputc('r', db);
    fputc(0x10, db); fputc(0x00, db);
    fputc(((langcode & 0xFF00) >> 8), db); fputc((langcode & 0xFF), db);
    /* do not know the number of messages yet; will update later */
    fputc(0x00, db); fputc(0x00, db); fputc(0x00, db); fputc(0x00, db);
    nmsgs = 0;
    do {
      if (read_line(base, (unsigned char*) &(buf[0])) != 0 ||
          read_line(trans, (unsigned char*) &(buf2[0])) != 0)
        break;
      if ((char) buf[0] == '\0' || (char) buf2[0] == '\0')
        continue;
      if (strcmp((char*) &(buf[0]), (char*) &(buf2[0])) == 0)
        continue;
      nmsgs++;
      fputc(0x81, db);
      fwrite((void*) &(buf[0]),
             (size_t) 1, strlen((char*) &(buf[0])) + (size_t) 1, db);
      fputc(0x82, db);
      fwrite((void*) &(buf2[0]),
             (size_t) 1, strlen((char*) &(buf2[0])) + (size_t) 1, db);
    } while (1);
    /* write number of messages */
    fseek(db, 8L, SEEK_SET);
    fputc(((nmsgs & 0x7F000000) >> 24), db);
    fputc(((nmsgs & 0x00FF0000) >> 16), db);
    fputc(((nmsgs & 0x0000FF00) >> 8), db);
    fputc((nmsgs & 0x000000FF), db);
    /* done creating database */
    fflush(db);
    fclose(base); fclose(trans); fclose(db);
    fprintf(stderr, "OK\n");
    return 0;
}

