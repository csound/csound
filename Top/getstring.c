/*  
    getstring.c:

    Copyright (C) 1999 John ffitch

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

#include "cs.h"

#if !defined XMGDIR
#define XMGDIR "/usr/local/lib"
#endif

static char **strings = NULL;

/*RWD*/
static int numstrings = 0;

static long baseloc;
static FILE *db;

extern long natlong(long);
#ifdef WIN32
extern char *getDB(void);
#endif
#ifdef mills_macintosh
Handle GETS128;

char *getstring(int n, char *st)
{
    long loc;
    int len;

    if (strings && strings[n]!=NULL) return strings[n];
    if (db == NULL) {
      unsigned char buff[255];
      GetIndString(buff, 128, n);
      PtoCString(buff);
      len = strlen((char*)buff)+1;
      strings[n] = (char*)malloc(len);
      strcpy(strings[n], (char*)buff);
      return strings[n];
    }
    fseek(db, n*sizeof(long)+baseloc, SEEK_SET);
    fread(&loc, sizeof(long), 1, db);
    loc = natlong(loc);
    if (loc==0) {
      return (strings[n] = "? ? ?");
    }
    fseek(db, loc, SEEK_SET);
    fread(&len, sizeof(int), 1, db);
    len = natlong(len);
    strings[n] = (char*)malloc(len);
    fread(strings[n], 1, len, db);
    return strings[n];
}
#else
char *getstring(int n, char *defaultStr)
{
    long loc;
    int len;

    if (strings && strings[n]!=NULL) return strings[n];
    if (db == NULL)
        return defaultStr;
    fseek(db, n*sizeof(long)+baseloc, SEEK_SET);
    fread(&loc, sizeof(long), 1, db);
    loc = natlong(loc);
    if (loc==0) {
      return (strings[n] = "? ? ?");
    }
    fseek(db, loc, SEEK_SET);
    fread(&len, sizeof(int), 1, db);
    len = natlong(len);
    strings[n] = (char*)malloc(len);
    fread(strings[n], 1, len, db);
    return strings[n];
}
#endif

void init_getstring(int argc, char **argv)
{
    char header[20];
    char lang[30];
    char *path;
    long maxnum;
    int j;
    int order;
    char name[256];
    db = NULL;
    /* First check arguments to see if it gives a DB to use */
    if (argv != NULL)
      for (j=1; j<argc; j++) {
        if (argv[j][0]=='-' && argv[j][1]=='j') {
          char *name = (argv[j][2]=='\0' ? argv[j+1] : &(argv[j][2]));
          db = fopen(name,  "rb");
          if (db == NULL) {
            err_printf( "failed to find DB file given in -j option.\n");
            exit(1);
          }
          else err_printf("Using %s\n", name);
          goto cmdlne;
        }
      }
#ifndef mills_macintosh
    if ((path=getenv("CSSTRNGS")) != NULL) {
      strcpy(name, path);
      db = fopen(path, "rb");
    }
#endif
#ifdef WIN32
    if (db==NULL)
# ifdef _CONSOLE
    {
/*    if (GetPrivateProfileString("csound", "StringDB", "",
                                  name, 256, "csound.ini"))  */
/*      db = fopen(name, "rb"); */
    }
# else
    {
      path = getDB();
      if (path) {
        strcpy(name, path);
        db = fopen(name, "rb");
      }
    }
# endif
#endif
    if (db==NULL) {
      strcpy(name, "csound.xmg");
      db = fopen("csound.xmg", "rb");
    }
    if (db == NULL && argv != NULL && argv[0] != NULL) {
      strcpy(name, argv[0]);
      if ((path = strrchr(name, '/')) != NULL
          || (path = strrchr(name, '\\')) != NULL) {
              strcpy(path+1, "csound.xmg");
              db = fopen(name, "rb");
      }
    }
    if (db == NULL && (path=getenv("SSDIR")) != NULL) {
      sprintf(name, "%s/csound.xmg", path);
      db = fopen(name, "rb");
    }
    if (db == NULL && (path=getenv("SFDIR")) != NULL) {
      sprintf(name, "%s/csound.xmg", path);
      db = fopen(name, "rb");
    }
    if (db == NULL && (path=getenv("SADIR")) != NULL) {
      sprintf(name, "%s/csound.xmg", path);
      db = fopen(name, "rb");
    }
    if (db == NULL) {
      path = XMGDIR;
      sprintf(name, "%s/csound.xmg", path);
      db = fopen(name, "rb");
    }
#ifdef mills_macintosh
    if (db == NULL) {
      err_printf("Using STR# 128\n");
      strings = (char**) malloc((maxnum=X_MAXNUM)*sizeof(char*));
      if (strings==NULL) {
        err_printf( "No memory for strings\n");
        exit(1);
      }
      for (j=0; j<maxnum; j++) strings[j] = NULL;
      return;
    }
#else
    if (db == NULL) {
      err_printf(
/*            "failed to find Strings DataBase file.\n" */
/*            "Use the CSSTRNGS environment variable or you must put the \n" */
/*            "file \"csound.xmg\" in your local, SFDIR, SADIR, SSDIR or \n" */
/*            "system directories for csound to work\n" */
              "Using default language\n");
/*       exit(1); */
      maxnum = X_MAXNUM;
      goto internal;
    }
    else err_printf("Using %s\n", name);
#endif
 cmdlne:
    fread(&order, sizeof(int), 1, db);
    fread(header, sizeof(X_HEADER)-1, 1, db);
    fread(lang, sizeof(char), 30, db);
    fread(&maxnum, sizeof(long), 1, db);
    maxnum = natlong(maxnum);
 internal:
    strings = (char**) malloc(maxnum*sizeof(char*));
    if (strings==NULL) {
      err_printf("No memory for strings\n");
      exit(1);
    }
    for (j=0; j<maxnum; j++) strings[j] = NULL;
    if (db) baseloc = ftell(db);
    /*RWD*/
    numstrings = maxnum;
}


/*RWD eliminate a big memory leak */
void free_strings(void)
{
    int i;
    if (strings) {
      for (i=0;i < numstrings;i++)
        if (strings[i] != NULL)
          free(strings[i]);
      free(strings);
      strings = NULL;
    }
}
