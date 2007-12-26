/*
    getstring.c:

    Copyright (C) 1999 John ffitch
    Jan 27 2005: replaced with new implementation by Istvan Varga
    Dec 25 2007: added GNU gettext implementation as alternative -- John ffitch

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

#include "csoundCore.h"

#ifdef HAVE_DIRENT_H
#  include <sys/types.h>
#  include <dirent.h>
#  if 0 && defined(__MACH__)
typedef void* DIR;
DIR opendir(const char *);
struct dirent *readdir(DIR*);
int closedir(DIR*);
#  endif
#endif

#include "namedins.h"

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

#ifdef GNU_GETTEXT
#define CSSTRNGS_VERSION 0x2000
#include <locale.h>

void init_getstring(void)
{
    const char  *s;

    s = csoundGetEnv(NULL, "CS_LANG");
    if (s == NULL)              /* Default locale */
      setlocale (LC_MESSAGES, "");
    else 
      setlocale (LC_MESSAGES, s);    /* Set to particular value */
    textdomain("csound5");
    /* bind_textdomain_codeset("csound5", "UTF-8"); */
#ifdef BETA
    /* This is experimental; where should these be?? */
    bindtextdomain("csound5", "/home/jpff/Sourceforge/csound5/po");
    printf("%s\n", setlocale(LC_MESSAGES, NULL));
#endif
}

PUBLIC char *csoundLocalizeString(const char *s)
{
    return gettext(s);
}
#else

#define CSSTRNGS_VERSION 0x1000

static const char *language_names[] = {
    "(default)",
    "Afrikaans",
    "Albanian",
    "Arabic",
    "Armenian",
    "Assamese",
    "Azeri",
    "Basque",
    "Belarusian",
    "Bengali",
    "Bulgarian",
    "Catalan",
    "Chinese",
    "Croatian",
    "Czech",
    "Danish",
    "Dutch",
    "English (UK)",
    "English (US)",
    "Estonian",
    "Faeroese",
    "Farsi",
    "Finnish",
    "French",
    "Georgian",
    "German",
    "Greek",
    "Gujarati",
    "Hebrew",
    "Hindi",
    "Hungarian",
    "Icelandic",
    "Indonesian",
    "Italian",
    "Japanese",
    "Kannada",
    "Kashmiri",
    "Kazak",
    "Konkani",
    "Korean",
    "Latvian",
    "Lithuanian",
    "Macedonian",
    "Malay",
    "Malayalam",
    "Manipuri",
    "Marathi",
    "Nepali",
    "Norwegian",
    "Oriya",
    "Polish",
    "Portuguese",
    "Punjabi",
    "Romanian",
    "Russian",
    "Sanskrit",
    "Serbian",
    "Sindhi",
    "Slovak",
    "Slovenian",
    "Spanish",
    "Swahili",
    "Swedish",
    "Tamil",
    "Tatar",
    "Telugu",
    "Thai",
    "Turkish",
    "Ukrainian",
    "Urdu",
    "Uzbek",
    "Vietnamese"
};

typedef struct lclstr_s {
    char    *str;                       /* original string      */
    char    *str_tran;                  /* translated string    */
    struct lclstr_s                     /* pointer to previous  */
            *prv;                       /*   structure in chain */
} lclstr_t;

typedef struct l10ndb_s {
    cslanguage_t    lang_code;          /* language code        */
    lclstr_t        *h_tabl[1024];      /* 10 bit hash table    */
} l10ndb_t;

static  l10ndb_t    getstr_db = { CSLANGUAGE_DEFAULT, { NULL } };

/* Set default language and free all memory used by string database. */

void csound_free_string_database(void)
{
    int         i;
    lclstr_t    *p, *q;

    getstr_db.lang_code = CSLANGUAGE_DEFAULT;
    for (i = 0; i < 1024; i++) {
      p = getstr_db.h_tabl[i];
      while (p != NULL) {
        if (p->str != NULL)
          free(p->str);
        if (p->str_tran != NULL)
          free(p->str_tran);
        q = p->prv;
        free(p);
        p = q;
      }
      getstr_db.h_tabl[i] = NULL;
    }
}

/* Translate string 's' to the current language, and return pointer to      */
/* the translated message.                                                  */
/* This may be the same as 's' if language was set to CSLANGUAGE_DEFAULT,   */
/* or the string is not found in the database.                              */

PUBLIC char *csoundLocalizeString(const char *s)
{
    /* if default language, */
    if (getstr_db.lang_code == CSLANGUAGE_DEFAULT)
      return (char*) s;         /* return original string */
    else {
      int       h;
      lclstr_t  *p;
      /* calculate hash value */
      h = (int) (csound_str_hash_32(s) & 0x03FFU);
      /* check table */
      p = getstr_db.h_tabl[h];
      while (p != NULL && sCmp(s, p->str)) p = p->prv;
      if (p == NULL) {
        return (char*) s;       /* no entry, return original string */
      }
      else {
        return (char*) p->str_tran;     /* otherwise translate */
      }
    }
}

#define   READ_HDR_BYTE                 \
{                                       \
    i = (getc(f));                      \
    if (i == EOF) goto end_of_file;     \
    j = (j << 8) | i;                   \
}

#define   READ_DATA_BYTE                                        \
{                                                               \
    i = (getc(f));                                              \
    if (i == EOF) {                                             \
      fprintf(stderr, "%s: unexpected end of file\n", fname);   \
      goto end_of_file;                                         \
    }                                                           \
}

/* Load a single string database file.   */
/* Returns the number of strings loaded. */

static int load_language_file(const char *fname, cslanguage_t lang_code)
{
    int     i, j, nr_strings = 0, nr_reqd, h, max_length = 16384;
    char    *buf1, *buf2;
    unsigned char   *c;
    lclstr_t        *p;
    FILE    *f;

    /* allocate memory for string buffers */
    buf1 = (char*) malloc((size_t) max_length * sizeof(char));
    if (buf1 == NULL) {
      fprintf(stderr, "csoundSetLanguage: not enough memory\n");
      return 0;
    }
    buf2 = (char*) malloc((size_t) max_length * sizeof(char));
    if (buf2 == NULL) {
      fprintf(stderr, "csoundSetLanguage: not enough memory\n");
      return 0;
    }
    /* open file */
    f = fopen(fname, "rb");
    if (f == NULL)
      return 0;
    /* check magic number (4 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j != 0x63537472)                /* "cStr" */
      goto end_of_file;
    /* check version (2 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j != CSSTRNGS_VERSION) {
      goto end_of_file;
    }
    /* language code (2 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j != (int) lang_code)
      goto end_of_file;
    /* number of strings (4 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j < 1) {
      fprintf(stderr, "csoundSetLanguage: %s: warning: no strings in file\n",
                      fname);
      goto end_of_file;
    }
    nr_reqd = j;
    /* read all strings from file */
    do {
      /* original string: */
      READ_DATA_BYTE;
      if (i != 0x81) {
        fprintf(stderr, "csoundSetLanguage: %s: corrupted file\n", fname);
        goto end_of_file;
      }
      c = (unsigned char*) buf1;
      j = 0;
      /* read, and calculate hash value */
      while (1) {
        READ_DATA_BYTE;
        *c++ = (unsigned char) i;
        if (!i) break;
        if (++j >= max_length) {
          fprintf(stderr, "csoundSetLanguage: %s: error: string length > %d\n",
                          fname, max_length);
          goto end_of_file;
        }
      }
      /* translated string: */
      READ_DATA_BYTE;
      if (i != 0x82) {
        fprintf(stderr, "csoundSetLanguage: %s: corrupted file\n", fname);
        goto end_of_file;
      }
      c = (unsigned char*) buf2;
      j = 0;
      /* read */
      do {
        if (j >= max_length) {
          fprintf(stderr, "csoundSetLanguage: %s: error: string length > %d\n",
                          fname, max_length);
          goto end_of_file;
        }
        READ_DATA_BYTE;
        *c++ = (unsigned char) i;
        j++;
      } while (i);
      /* check database: if string is already defined, ignore new definition */
      h = (int) (csound_str_hash_32(&(buf1[0])) & 0x03FFU);
      p = getstr_db.h_tabl[h];
      while (p != NULL && strcmp(p->str, buf1) != 0) p = p->prv;
      if (p != NULL) continue;
      /* store in database */
      p = (lclstr_t*) malloc(sizeof(lclstr_t));
      if (p == NULL) {
        fprintf(stderr, "csoundSetLanguage: not enough memory\n");
        goto end_of_file;
      }
      p->str = (char*) malloc((size_t) strlen(buf1) + (size_t) 1);
      if (p->str == NULL) {
        free(p);
        fprintf(stderr, "csoundSetLanguage: not enough memory\n");
        goto end_of_file;
      }
      p->str_tran = (char*) malloc((size_t) strlen(buf2) + (size_t) 1);
      if (p->str_tran == NULL) {
        free(p->str); free(p);
        fprintf(stderr, "csoundSetLanguage: not enough memory\n");
        goto end_of_file;
      }
      strcpy(p->str, buf1);
      strcpy(p->str_tran, buf2);
      p->prv = getstr_db.h_tabl[h];
      getstr_db.h_tabl[h] = p;
      nr_strings++;             /* successfully loaded */
    } while (--nr_reqd);

 end_of_file:
    fclose(f);
    free(buf2);
    free(buf1);
    return nr_strings;
}

/* Set language to 'lang_code' (lang_code can be for example            */
/* CSLANGUAGE_ENGLISH_UK or CSLANGUAGE_FRENCH or many others, see       */
/* n_getstr.h for the list of languages).                               */
/* This affects all Csound instances running in the address space       */
/* of the current process.                                              */
/* The special language code CSLANGUAGE_DEFAULT can be used to disable  */
/* translation of messages and free all memory allocated by a previous  */
/* call to csoundSetLanguage().                                         */
/* csoundSetLanguage() loads all files for the selected language from   */
/* the directory specified by the CSSTRNGS environment variable.        */

PUBLIC void csoundSetLanguage(cslanguage_t lang_code)
{
    int         nr_strings = 0;
    const char  *dirnam;
#ifdef HAVE_DIRENT_H
    int         i;
    char        *s, dir_name[1024], file_name[1024];
    DIR         *dirptr;
    struct dirent *ent;
#endif

    if (lang_code == CSLANGUAGE_DEFAULT)
      fprintf(stderr, "Localisation of messages is disabled, using "
                      "default language.\n");
    else
      fprintf(stderr, "Setting language of messages to %s ...\n",
                      language_names[(int) lang_code]);
    if (getstr_db.lang_code == lang_code)
      return;

    csound_free_string_database();
    /* set language code */
    getstr_db.lang_code = lang_code;
    if (lang_code == CSLANGUAGE_DEFAULT)
      return;

    /* load all files from CSSTRNGS directory */
    dirnam = csoundGetEnv(NULL, "CSSTRNGS");
    if (dirnam == NULL)
      return;

#ifdef HAVE_DIRENT_H
    /* directory name */
    strcpy((char*) dir_name, dirnam);
    i = strlen(dir_name);
    if (i > 0) {
      /* strip any trailing pathname delimiter */
      if (dir_name[i - 1] == DIRSEP || dir_name[i - 1] == '/') i--;
      /* and now add one to make sure there is exactly one */
      dir_name[i++] = DIRSEP;
      dir_name[i] = '\0';
    }
    dirptr = opendir(dirnam);
    if (dirptr == NULL)
      return;
    while ((ent = readdir(dirptr)) != NULL) {
      s = ent->d_name;                  /* file name        */
      strcpy(file_name, dir_name);      /* with full path   */
      strcat(file_name, s);
      nr_strings += load_language_file(file_name, lang_code);
    }
    closedir(dirptr);
#else
    /* if dirent.h is not available, CSSTRNGS is expected to */
    /* point to a single file */
    nr_strings += load_language_file(dirnam, lang_code);
#endif
    fprintf(stderr, " ... done, %d strings loaded.\n", nr_strings);
}

/* ------------------------------------------------------------------------- */

typedef struct LanguageSpec_s {
    char    *langname;
    int     langcode;
} LanguageSpec_t;

static LanguageSpec_t lang_list[] = {
    { "English_UK",     CSLANGUAGE_ENGLISH_UK   },
    { "uk",             CSLANGUAGE_ENGLISH_UK   },
    { "English",        CSLANGUAGE_ENGLISH_UK   },
    { "English_US",     CSLANGUAGE_ENGLISH_US   },
    { "us",             CSLANGUAGE_ENGLISH_US   },
    { "French",         CSLANGUAGE_FRENCH       },
    { "fr",             CSLANGUAGE_FRENCH       },
    { "German",         CSLANGUAGE_GERMAN       },
    { "de",             CSLANGUAGE_GERMAN       },
    { "Italian",        CSLANGUAGE_ITALIAN      },
    { "it",             CSLANGUAGE_ITALIAN      },
    { "Spanish",        CSLANGUAGE_SPANISH      },
    { "es",             CSLANGUAGE_SPANISH      },
    { NULL, -1 },
};

void init_getstring(void)
{
    const char  *s;
    int         n;

    s = csoundGetEnv(NULL, "CS_LANG");
    if (s == NULL)
      csoundSetLanguage(CSLANGUAGE_DEFAULT);
    else {
      n = -1;
      while (lang_list[++n].langname != NULL) {
        if (strcmp(s, lang_list[n].langname) == 0)
          break;
      }
      if (lang_list[n].langname != NULL)
        n = lang_list[n].langcode;
      else
        n = CSLANGUAGE_DEFAULT;
      csoundSetLanguage(n);
    }
}

#endif
