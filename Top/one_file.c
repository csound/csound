/*
    one_file.c:

    Copyright (C) 1998 John ffitch

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

#include "cs.h"
#include "csound.h"
#include <ctype.h>

static char buffer[200];
#ifdef WIN32
#undef L_tmpnam
#define L_tmpnam (200)
#endif
static char orcname[L_tmpnam+4];
       char sconame[L_tmpnam+4];
static char midname[L_tmpnam+4];
static int midiSet;
#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

typedef struct namelst {
  char           *name;
  struct namelst *next;
} NAMELST;

static NAMELST *toremove = NULL;

#ifdef WIN32
char *mytmpnam(char *a)
{
    char *dir = getenv("SFDIR");
    if (dir==NULL) dir=getenv("HOME");
    dir = _tempnam(dir, "cs");
    strcpy(a, dir);
    free(dir);
    return a;
}
#endif

#ifdef mills_macintosh
#define fgets macgetstring
char macBuffer[200];
int macBufNdx = 200;
char *macgetstring(char *str, int num, FILE *stream)
{
    int bufferNdx = 0;
    size_t ourReturn;
    while (true) {
      if (macBufNdx >= 200) { /*then we must read in new buffer */
        macBufNdx = 0;
        ourReturn = fread(macBuffer, 1, num, stream);
        if (ourReturn == 0)
          return NULL;
      }
      else {
        char c = macBuffer[macBufNdx];
        if (c == '\0' || c == '\n' || c == '\r') {
          buffer[bufferNdx] = '\r';
          if (bufferNdx < 199)
            buffer[bufferNdx+1] = '\0';
          macBufNdx++;
          return buffer;
        }
        else {
          buffer[bufferNdx] = c;
          bufferNdx++;
          macBufNdx++;
        }
      }
    }
}
#endif

static char *my_fgets(char *s, int n, FILE *stream)
{
    char *a = s;
    if (n <= 1) return NULL;                  /* best of a bad deal */
    do {
      int ch = getc(stream);
      if (ch == EOF) {                       /* error or EOF       */
        if (s == a) return NULL;         /* no chars -> leave  */
        if (ferror(stream)) a = NULL;
        break; /* add NULL even if ferror(), spec says 'indeterminate' */
      }
      if ((*s++ = ch) == '\n') break;
      if (*(s-1) == '\r') break;
    }
    while (--n > 1);
    *s = 0;
    return a;
}

int firstsamp = 1;
int sampused[100];

void remove_tmpfiles (void)             /* IV - Oct 31 2002 */
{                             /* use one fn to delete all temporary files */
    while (toremove) {
      NAMELST *nxt = toremove->next;
#ifdef BETA
      err_printf("Removing temporary file %s ...\n", toremove->name);
#endif
      if (remove(toremove->name))
        err_printf(Str("WARNING: could not remove %s\n"), toremove->name);
      mfree(toremove->name);
      mfree(toremove);
      toremove = nxt;
    }
}

void add_tmpfile (char *name)           /* IV - Oct 31 2002 */
{                      /* add temporary file to delete list */
    NAMELST *tmp = (NAMELST*) mmalloc(sizeof(NAMELST));
    tmp->name = mmalloc(strlen(name) + 1);
    strcpy(tmp->name, name);
    tmp->next = toremove;
    toremove = tmp;
}

static char files[1000];
extern int argdecode(void*, int, char**, char**, char*);

int readOptions(void *csound, FILE *unf)
{
    char *p;
    int argc = 0;
    char *argv[100];
    char *filnamp = files;

    while (my_fgets(buffer, 200, unf)!= NULL) {
      p = buffer;
      while (*p==' ' || *p=='\t') p++;
      if (strstr(p,"</CsOptions>") == buffer) {
        return TRUE;
      }
      /**
       * Allow command options in unified CSD files
       * to begin with the Csound command, so that
       * the command line arguments can be exactly the same in unified files
       * as for regular command line invocation.
       */
      orchname = 0;
      if (*p==';' || *p=='#') continue; /* Comment line? */
      if (*p=='\n' || *p=='\r') continue; /* Empty line? */
      argc = 0;
      argv[0] = p;
      while (*p==' ') p++;      /* Ignore leading space */
      if (*p=='-') {            /* Deal with case where no command name is given */
        argv[0] = "csound";
        argv[1] = p;
        argc++;
      }
      while (*p != '\0') {
        if (*p==' ') {
          *p++ = '\0';
#ifdef _DEBUG
          printf("argc=%d argv[%d]=%s\n", argc, argc, argv[argc]);
#endif
          while (*p == ' ') p++;
          if (*p==';' ||
              *p=='#' ||
              (*p == '/' && *(p+1) == '/')) { /* Comment line? */
            *p = '\0'; break;
          }
          if (*p == '/' && *(p+1) == '*') {  /* Comment line? */
            p += 2;
          top:
            while (*p != '*' && *p != '\0') p++;
            if (*p == '*' && *(p+1)== '/') {
              p += 2; break;
            }
            if (*p=='*') {
              p++; goto top;
            }
            my_fgets(buffer, 200, unf);
            p = buffer; goto top;
          }
          argv[++argc] = p;
        }
        else if (*p=='\n' || *p == '\r') {
          *p = '\0';
          break;
        }
        p++;
      }
#ifdef _DEBUG
      printf("argc=%d argv[%d]=%s\n", argc, argc, argv[argc]);
#endif
      /*      argc++; */                  /* according to Nicola but wrong */
      /* Read an argv thing */
      argdecode(csound, argc, argv, &filnamp, getenv("SFOUTYP"));
    }
    return FALSE;
}

static int createOrchestra(FILE *unf)
{
    char *p;
    FILE *orcf;

    tmpnam(orcname);            /* Generate orchestra name */
    if ((p=strchr(orcname, '.')) != NULL) *p='\0'; /* with extention */
    strcat(orcname, ".orc");
    orcf = fopen(orcname, "w");
    printf(Str("Creating %s (%p)\n"), orcname, orcf);
    if (orcf==NULL) {
      perror(Str("Failed to create\n"));
      longjmp(cenviron.exitjmp_,1);
    }
    while (my_fgets(buffer, 200, unf)!= NULL) {
      p = buffer;
      while (*p==' '||*p=='\t') p++;
      if (strstr(p,"</CsInstruments>") == buffer) {
        fclose(orcf);
        add_tmpfile(orcname);           /* IV - Oct 31 2002 */
        return TRUE;
      }
      else fputs(buffer, orcf);
    }
    return FALSE;
}


static int createScore(FILE *unf)
{
    char *p;
    FILE *scof;

    tmpnam(sconame);            /* Generate score name */
    if ((p=strchr(sconame, '.')) != NULL) *p='\0'; /* with extention */
    strcat(sconame, ".sco");
    scof = fopen(sconame, "w");
        /*RWD 3:2000*/
    if (scof==NULL)
      return FALSE;

    while (my_fgets(buffer, 200, unf)!= NULL) {
      p = buffer;
      while (*p==' '||*p=='\t') p++;
     if (strstr(p,"</CsScore>") == buffer) {
        fclose(scof);
        add_tmpfile(sconame);           /* IV - Oct 31 2002 */
        return TRUE;
      }
      else fputs(buffer, scof);
    }
    return FALSE;
}

static int createMIDI(FILE *unf)
{
    int size;
    char *p;
    FILE *midf;
    int c;

    if (tmpnam(midname)==NULL) { /* Generate MIDI file name */
      printf(Str("Cannot create temporary file for MIDI subfile\n"));
      longjmp(cenviron.exitjmp_,1);
    }
    if ((p=strchr(midname, '.')) != NULL) *p='\0'; /* with extention */
    strcat(midname, ".mid");
    midf = fopen(midname, "wb");
    if (midf==NULL) {
      printf(Str("Cannot open temporary file (%s) for MIDI subfile\n"),
             midname);
      longjmp(cenviron.exitjmp_,1);
    }
    my_fgets(buffer, 200, unf);
    if (sscanf(buffer, Str("Size = %d"), &size)==0) {
      printf(Str("Error in reading MIDI subfile -- no size read\n"));
      longjmp(cenviron.exitjmp_,1);
    }
    for (; size > 0; size--) {
      c = getc(unf);
      putc(c, midf);
    }
    fclose(midf);
    add_tmpfile(midname);               /* IV - Oct 31 2002 */
    midiSet = TRUE;
    while (TRUE) {
      if (my_fgets(buffer, 200, unf)!= NULL) {
        p = buffer;
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsMidifile>") == buffer) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static void read_base64(FILE *in, FILE *out)
{
    int c;
    int cycl = 0;
    int n[4];

    do {
      c = getc(in);
    } while (c==' ' || c=='\n');
    ungetc(c, in);
    while ((c = getc(in)) != '=' && c != '<') {
      while (c == '\n') c = getc(in);
      if (c == '=' || c == '<' || c == EOF) break;
      if (isupper(c))       n[cycl] = c-'A';
      else if (islower(c))  n[cycl] = c-'a'+26;
      else if (isdigit(c))  n[cycl] = c-'0'+52;
      else if (c == '+')    n[cycl] = 62;
      else if (c == '/')    n[cycl] = 63;
      else {
        err_printf( "Non 64base character %c(%2x)\n", c, c);
        longjmp(cenviron.exitjmp_,1);
      }
      cycl++;
      if (cycl == 4) {
        putc((n[0] << 2) | (n[1] >> 4), out);
        putc(((n[1] & 0xf) <<4) | ((n[2] >> 2) & 0xf), out);
        putc(((n[2] & 0x3) << 6) | n[3], out);
        cycl = 0;
      }
    }
    if (c=='<') ungetc(c, in);
    if (cycl == 1) {
      err_printf("Ended on cycl=1\n");
    }
    else if (cycl == 2) {
      putc((n[0] << 2) | (n[1] >> 4), out);
    }
    else if (cycl == 3) {
        putc((n[0] << 2) | (n[1] >> 4), out);
        putc(((n[1] & 0xf) <<4) | ((n[2] >> 2) & 0xf), out);
    }
}

static int createMIDI2(FILE *unf)
{
    char *p;
    FILE *midf;

    if (tmpnam(midname)==NULL) { /* Generate MIDI file name */
      printf(Str("Cannot create temporary file for MIDI subfile\n"));
      longjmp(cenviron.exitjmp_,1);
    }
    if ((p=strchr(midname, '.')) != NULL) *p='\0'; /* with extention */
    strcat(midname, ".mid");
    midf = fopen(midname, "wb");
    if (midf==NULL) {
      printf(Str("Cannot open temporary file (%s) for MIDI subfile\n"),
             midname);
      longjmp(cenviron.exitjmp_,1);
    }
    read_base64(unf, midf);
    fclose(midf);
    add_tmpfile(midname);               /* IV - Oct 31 2002 */
    midiSet = TRUE;
    while (TRUE) {
      if (my_fgets(buffer, 200, unf)!= NULL) {
        p = buffer;
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsMidifileB>") == buffer) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static int createSample(FILE *unf)
{
    int num;
    FILE *smpf;
    char sampname[256];

    sscanf(buffer, "<CsSampleB filename=%d>", &num);
    sprintf(sampname, "soundin.%d", num);
    if ((smpf=fopen(sampname, "r")) !=NULL) {
      printf(Str("File %s already exists\n"), sampname);
      longjmp(cenviron.exitjmp_,1);
    }
    fclose(smpf);
    smpf = fopen(sampname, "wb");
    if (smpf==NULL) {
      printf(Str("Cannot open sample file (%s) subfile\n"), sampname);
      longjmp(cenviron.exitjmp_,1);
    }
    read_base64(unf, smpf);
    fclose(smpf);
    sampused[num] = 1;          /* Remember to delete */
    if (firstsamp) firstsamp = 0;
    add_tmpfile(sampname);              /* IV - Oct 31 2002 */
    while (TRUE) {
      if (my_fgets(buffer, 200, unf)!= NULL) {
        char *p = buffer;
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsSampleB>") == buffer) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static int createFile(FILE *unf)
{
    FILE *smpf;
    char filename[256];

    sscanf(buffer, "<CsFileB filename=%s>", filename);
    if ((smpf=fopen(filename, "r")) !=NULL) {
      printf(Str("File %s already exists\n"), filename);
      longjmp(cenviron.exitjmp_,1);
    }
    fclose(smpf);
    smpf = fopen(filename, "wb");
    if (smpf==NULL) {
      printf(Str("Cannot open file (%s) subfile\n"), filename);
      longjmp(cenviron.exitjmp_,1);
    }
    read_base64(unf, smpf);
    fclose(smpf);
    if (firstsamp) firstsamp = 0;
    add_tmpfile(filename);              /* IV - Oct 31 2002 */

    while (TRUE) {
      if (my_fgets(buffer, 200, unf)!= NULL) {
        char *p = buffer;
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsFileB>") == buffer) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static int checkVersion(FILE *unf)
{
    char *p;
    int major = 0, minor = 0;
    int result = TRUE;
    int version = csoundGetVersion();
    while (my_fgets(buffer, 200, unf)!= NULL) {
      p = buffer;
      while (*p==' '||*p=='\t') p++;
      if (strstr(p, "</CsVersion>")==0)
        return result;
      if (strstr(p, "Before")==0) {
        sscanf(p, "Before %d.%d", &major, &minor);
        if (version > ((major * 100) + minor))
          result = FALSE;
      }
      else if (strstr(p, "After")==0) {
        sscanf(p, "After %d.%d", &major, &minor);
        if (version < ((major * 100) + minor))
          result = FALSE;
      }
      else if (sscanf(p, "%d.%d", &major, &minor)==2) {
        sscanf(p, "Before %d.%d", &major, &minor);
        if (version > ((major * 100) + minor))
          result = FALSE;
      }
    }
    return result;
}

static int eat_to_eol(char *buf)
{
    int i=0;
    while(buf[i] != '\n' && buf[i] != '\r') i++;
    return i;   /* keep the \n for further processing */
}

int blank_buffer(void)
{
    int i=0;
    if (buffer[i] == ';')
      i += eat_to_eol(&buffer[i]);
    while (buffer[i]!='\n' && buffer[i]!='\0') {
      if (buffer[i]!=' ' && buffer[i]!='\t') return FALSE;
      i++;
    }
    return TRUE;
}

int read_unified_file(void *csound, char **pname, char **score)
{
    char *name = *pname;
    FILE *unf  = fopen(name, "rb"); /* Need to open in binary to deal with
                                       MIDI and the like. */
    int result = TRUE;
    int started = FALSE;
    int r;
        /*RWD 3:2000  fopen can fail...*/
    if (unf==NULL)
      return 0;

    orcname[0] = sconame[0] = midname[0] = '\0';
    midiSet = FALSE;
    firstsamp = 1;
    memset(sampused, 0, 100*sizeof(int));
    /*    toremove = NULL;                            IV - Oct 31 2002 */
#ifdef _DEBUG
    printf("Calling unified file system with %s\n", name);
#endif
    while (my_fgets(buffer, 200, unf)) {
      char *p = buffer;
      while (*p==' '||*p=='\t') p++;
      if (strstr(p,"<CsoundSynthesizer>") == buffer ||
          strstr(p,"<CsoundSynthesiser>") == buffer) {
        printf(Str("STARTING FILE\n"));
        started = TRUE;
      }
      else if (strstr(p,"</CsoundSynthesizer>") == buffer ||
               strstr(p,"</CsoundSynthesiser>") == buffer) {
        *pname = orcname;
        *score = sconame;
        if (midiSet) {
          O.FMidiname = midname;
          O.FMidiin = 1;
        }
        fclose(unf);
        return result;
      }
      else if (strstr(p,"<CsOptions>") == buffer) {
        printf(Str("Creating options\n"));
        r = readOptions(csound, unf);
        result = r && result;
      }
/*        else if (strstr(p,"<CsFunctions>") == buffer) { */
/*      importFunctions(unf); */
/*        } */
      else if (strstr(p,"<CsInstruments>") == buffer) {
        printf(Str("Creating orchestra\n"));
        r = createOrchestra(unf);
        result = r && result;
      }
/*        else if (strstr(p,"<CsArrangement>") == buffer) { */
/*          importArrangement(unf); */
/*        } */
      else if (strstr(p,"<CsScore>") == buffer) {
        printf(Str("Creating score\n"));
        r = createScore(unf);
        result = r && result;
      }
/*        else if (strstr(p,"<CsTestScore>") == buffer) { */
/*          importTestScore(unf); */
/*        } */
      else if (strstr(p,"<CsMidifile>") == buffer) {
        r = createMIDI(unf);
        result = r && result;
      }
      else if (strstr(p,"<CsMidifileB>") == buffer) {
        r = createMIDI2(unf);
        result = r && result;
      }
      else if (strstr(p,"<CsSampleB filename=") == buffer) {
        r = createSample(unf);
        result = r && result;
      }
      else if (strstr(p,"<CsFileB filename=") == buffer) {
        r = createFile(unf);
        result = r && result;
      }
      else if (strstr(p,"<CsVersion>") == buffer) {
        r = checkVersion(unf);
        result = r && result;
      }
      else if (blank_buffer()) continue;
      else if (started && strchr(p, '<') == buffer) {
        printf(Str("unknown command :%s\n"), buffer);
      }
      else {                    /* Quietly skip unknown text */
        /* printf(Str("unknown command :%s\n"), buffer); */
      }
    }
    *pname = orcname;
    *score = sconame;
    if (midiSet) {
      O.FMidiname = midname;
      O.FMidiin = 1;
    }
    fclose(unf);
    return result;
}
