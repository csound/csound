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

#include "csoundCore.h"
#include <ctype.h>
#include <errno.h>

#define CSD_MAX_LINE_LEN    4096

#ifdef WIN32
#undef L_tmpnam
#define L_tmpnam (200)
#endif

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

typedef struct namelst {
  char           *name;
  struct namelst *next;
} NAMELST;

typedef struct {
    char    buffer[CSD_MAX_LINE_LEN];
    NAMELST *toremove;
    char    orcname[L_tmpnam + 4];
    char    sconame[L_tmpnam + 4];
    char    midname[L_tmpnam + 4];
    int     midiSet;
} ONE_FILE_GLOBALS;

#define ST(x)   (((ONE_FILE_GLOBALS*) csound->oneFileGlobals)->x)

#ifdef WIN32
char *mytmpnam(CSOUND *csound, char *a)
{
    char *dir = (char*) csoundGetEnv(csound, "SFDIR");
    if (dir == NULL)
      dir = (char*) csoundGetEnv(csound, "HOME");
    dir = _tempnam(dir, "cs");
    strcpy(a, dir);
    free(dir);
    return a;
}
#else
char *mytmpnam(CSOUND *csound, char *a)
{
    return tmpnam(a);
}
#endif

static void alloc_globals(CSOUND *csound)
{
    if (csound->oneFileGlobals == NULL) {
      csound->oneFileGlobals = mcalloc(csound, sizeof(ONE_FILE_GLOBALS));
    }
}

char *get_sconame(CSOUND *csound)
{
    alloc_globals(csound);
    return ST(sconame);
}

static char *my_fgets(char *s, int n, FILE *stream)
{
    char *a = s;
    if (n <= 1) return NULL;                 /* best of a bad deal */
    do {
      int ch = getc(stream);
      if (ch == EOF) {                       /* error or EOF       */
        if (s == a) return NULL;             /* no chars -> leave  */
        if (ferror(stream)) a = NULL;
        break; /* add NULL even if ferror(), spec says 'indeterminate' */
      }
      if (ch == '\n' || ch == '\r') {   /* end of line ? */
        *(s++) = '\n';                  /* convert */
        if (ch == '\r') {
          ch = getc(stream);
          if (ch != '\n')               /* Mac format */
            ungetc(ch, stream);
        }
        break;
      }
      *(s++) = ch;
    } while (--n > 1);
    *s = '\0';
    return a;
}

void remove_tmpfiles(CSOUND *csound)            /* IV - Feb 03 2005 */
{                               /* use one fn to delete all temporary files */
    alloc_globals(csound);
    while (ST(toremove) != NULL) {
      NAMELST *nxt = ST(toremove)->next;
#ifdef BETA
      csoundMessage(csound, Str("Removing temporary file %s ...\n"),
                            ST(toremove)->name);
#endif
      if (remove(ST(toremove)->name))
        csoundMessage(csound, Str("WARNING: could not remove %s\n"),
                              ST(toremove)->name);
      mfree(csound, ST(toremove)->name);
      mfree(csound, ST(toremove));
      ST(toremove) = nxt;
    }
}

void add_tmpfile(CSOUND *csound, char *name)    /* IV - Feb 03 2005 */
{                               /* add temporary file to delete list */
    NAMELST *tmp;
    alloc_globals(csound);
    tmp = (NAMELST*) mmalloc(csound, sizeof(NAMELST));
    tmp->name = (char*) mmalloc(csound, strlen(name) + 1);
    strcpy(tmp->name, name);
    tmp->next = ST(toremove);
    ST(toremove) = tmp;
}

extern int argdecode(CSOUND*, int, char**);

int readOptions(CSOUND *csound, FILE *unf)
{
    char *p;
    int argc = 0;
    char *argv[100];

    alloc_globals(csound);
    while (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
      p = ST(buffer);
      while (*p==' ' || *p=='\t') p++;
      if (strstr(p,"</CsOptions>") == ST(buffer)) {
        return TRUE;
      }
      /**
       * Allow command options in unified CSD files
       * to begin with the Csound command, so that
       * the command line arguments can be exactly the same in unified files
       * as for regular command line invocation.
       */
      if (*p==';' || *p=='#' || *p=='\n') continue; /* empty or comment line? */
      argc = 0;
      argv[0] = p;
      while (*p==' ' || *p=='\t') p++;  /* Ignore leading space */
      if (*p=='-') {        /* Deal with case where no command name is given */
        argv[0] = "csound";
        argv[1] = p;
        argc++;
      }
      while (*p != '\0') {
        if (*p==' ' || *p=='\t') {
          *p++ = '\0';
#ifdef _DEBUG
          csoundMessage(csound, "argc=%d argv[%d]=%s\n",
                                argc, argc, argv[argc]);
#endif
          while (*p == ' ' || *p=='\t') p++;
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
            my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf);
            p = ST(buffer); goto top;
          }
          argv[++argc] = p;
        }
        else if (*p=='\n') {
          *p = '\0';
          break;
        }
        p++;
      }
#ifdef _DEBUG
      csoundMessage(csound, "argc=%d argv[%d]=%s\n", argc, argc, argv[argc]);
#endif
      /*      argc++; */                  /* according to Nicola but wrong */
      /* Read an argv thing */
      argdecode(csound, argc, argv);
    }
    return FALSE;
}

static int createOrchestra(CSOUND *csound, FILE *unf)
{
    char  *p;
    FILE  *orcf;
    void  *fd;

    mytmpnam(csound, ST(orcname));            /* Generate orchestra name */
    if ((p=strchr(ST(orcname), '.')) != NULL) *p='\0'; /* with extention */
    strcat(ST(orcname), ".orc");
    fd = csoundFileOpen(csound, &orcf, CSFILE_STD, ST(orcname), "w", NULL);
    csoundMessage(csound, Str("Creating %s (%p)\n"), ST(orcname), orcf);
    if (fd == NULL) {
      csoundDie(csound, Str("Failed to create %s"), ST(orcname));
    }
    while (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
      p = ST(buffer);
      while (*p == ' ' || *p == '\t') p++;
      if (strstr(p,"</CsInstruments>") == ST(buffer)) {
        csoundFileClose(csound, fd);
        add_tmpfile(csound, ST(orcname));           /* IV - Feb 03 2005 */
        return TRUE;
      }
      else fputs(ST(buffer), orcf);
    }
    return FALSE;
}

static int createScore(CSOUND *csound, FILE *unf)
{
    char  *p;
    FILE  *scof;
    void  *fd;

    mytmpnam(csound, ST(sconame));            /* Generate score name */
    if ((p=strchr(ST(sconame), '.')) != NULL) *p='\0'; /* with extention */
    strcat(ST(sconame), ".sco");
    fd = csoundFileOpen(csound, &scof, CSFILE_STD, ST(sconame), "w", NULL);
    /* RWD 3:2000 */
    if (fd == NULL)
      return FALSE;

    while (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
      p = ST(buffer);
      while (*p==' '||*p=='\t') p++;
     if (strstr(p,"</CsScore>") == ST(buffer)) {
        csoundFileClose(csound, fd);
        add_tmpfile(csound, ST(sconame));           /* IV - Feb 03 2005 */
        return TRUE;
      }
      else fputs(ST(buffer), scof);
    }
    return FALSE;
}

static int createMIDI(CSOUND *csound, FILE *unf)
{
    int   size, c;
    char  *p;
    FILE  *midf;
    void  *fd;

    if (mytmpnam(csound, ST(midname))==NULL) { /* Generate MIDI file name */
      csoundDie(csound, Str("Cannot create temporary file for MIDI subfile"));
    }
    if ((p=strchr(ST(midname), '.')) != NULL) *p='\0'; /* with extention */
    strcat(ST(midname), ".mid");
    fd = csoundFileOpen(csound, &midf, CSFILE_STD, ST(midname), "wb", NULL);
    if (fd == NULL) {
      csoundDie(csound, Str("Cannot open temporary file (%s) for MIDI subfile"),
                        ST(midname));
    }
    my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf);
    if (sscanf(ST(buffer), Str("Size = %d"), &size)==0) {
      csoundDie(csound, Str("Error in reading MIDI subfile -- no size read"));
    }
    for (; size > 0; size--) {
      c = getc(unf);
      putc(c, midf);
    }
    csoundFileClose(csound, fd);
    add_tmpfile(csound, ST(midname));               /* IV - Feb 03 2005 */
    ST(midiSet) = TRUE;
    while (TRUE) {
      if (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
        p = ST(buffer);
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsMidifile>") == ST(buffer)) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static void read_base64(CSOUND *csound, FILE *in, FILE *out)
{
    int c;
    int n, nbits;

    n = nbits = 0;
    while ((c = getc(in)) != '=' && c != '<') {
      while (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        c = getc(in);
      if (c == '=' || c == '<' || c == EOF)
        break;
      n <<= 6;
      nbits += 6;
      if (isupper(c))
        c -= 'A';
      else if (islower(c))
        c -= ((int) 'a' - 26);
      else if (isdigit(c))
        c -= ((int) '0' - 52);
      else if (c == '+')
        c = 62;
      else if (c == '/')
        c = 63;
      else {
        csoundDie(csound, Str("Non base64 character %c(%2x)"), c, c);
      }
      n |= (c & 0x3F);
      if (nbits >= 8) {
        nbits -= 8;
        c = (n >> nbits) & 0xFF;
        n &= ((1 << nbits) - 1);
        putc(c, out);
      }
    }
    if (c == '<')
      ungetc(c, in);
    if (nbits >= 8) {
      nbits -= 8;
      c = (n >> nbits) & 0xFF;
      n &= ((1 << nbits) - 1);
      putc(c, out);
    }
    if (nbits > 0 && n != 0) {
      csoundDie(csound, Str("Truncated byte at end of base64 stream"));
    }
}

static int createMIDI2(CSOUND *csound, FILE *unf)
{
    char  *p;
    FILE  *midf;
    void  *fd;

    if (mytmpnam(csound, ST(midname))==NULL) { /* Generate MIDI file name */
      csoundDie(csound, Str("Cannot create temporary file for MIDI subfile"));
    }
    if ((p=strchr(ST(midname), '.')) != NULL) *p='\0'; /* with extention */
    strcat(ST(midname), ".mid");
    fd = csoundFileOpen(csound, &midf, CSFILE_STD, ST(midname), "wb", NULL);
    if (fd == NULL) {
      csoundDie(csound, Str("Cannot open temporary file (%s) for MIDI subfile"),
                        ST(midname));
    }
    read_base64(csound, unf, midf);
    csoundFileClose(csound, fd);
    add_tmpfile(csound, ST(midname));               /* IV - Feb 03 2005 */
    ST(midiSet) = TRUE;
    while (TRUE) {
      if (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
        p = ST(buffer);
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsMidifileB>") == ST(buffer)) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static int createSample(CSOUND *csound, FILE *unf)
{
    int   num;
    FILE  *smpf;
    void  *fd;
    char  sampname[256];

    sscanf(ST(buffer), "<CsSampleB filename=%d>", &num);
    sprintf(sampname, "soundin.%d", num);
    if ((smpf = fopen(sampname, "rb")) != NULL) {
      fclose(smpf);
      csoundDie(csound, Str("File %s already exists"), sampname);
    }
    fd = csoundFileOpen(csound, &smpf, CSFILE_STD, sampname, "wb", NULL);
    if (fd == NULL) {
      csoundDie(csound, Str("Cannot open sample file (%s) subfile"), sampname);
    }
    read_base64(csound, unf, smpf);
    csoundFileClose(csound, fd);
    add_tmpfile(csound, sampname);              /* IV - Feb 03 2005 */
    while (TRUE) {
      if (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
        char *p = ST(buffer);
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsSampleB>") == ST(buffer)) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static int createFile(CSOUND *csound, FILE *unf)
{
    FILE  *smpf;
    void  *fd;
    char  filename[256];

    filename[0] = '\0';
    sscanf(ST(buffer), "<CsFileB filename=%s>", filename);
    if (filename[0] != '\0' && filename[strlen(filename) - 1] == '>')
      filename[strlen(filename) - 1] = '\0';
    if ((smpf = fopen(filename, "rb")) != NULL) {
      fclose(smpf);
      csoundDie(csound, Str("File %s already exists"), filename);
    }
    fd = csoundFileOpen(csound, &smpf, CSFILE_STD, filename, "wb", NULL);
    if (fd == NULL) {
      csoundDie(csound, Str("Cannot open file (%s) subfile"), filename);
    }
    read_base64(csound, unf, smpf);
    csoundFileClose(csound, fd);
    add_tmpfile(csound, filename);              /* IV - Feb 03 2005 */

    while (TRUE) {
      if (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
        char *p = ST(buffer);
        while (*p==' '||*p=='\t') p++;
        if (strstr(p,"</CsFileB>") == ST(buffer)) {
          return TRUE;
        }
      }
    }
    return FALSE;
}

static int checkVersion(CSOUND *csound, FILE *unf)
{
    char  *p;
    int   major = 0, minor = 0;
    int   result = TRUE;
    int   version = csoundGetVersion();

    while (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)!= NULL) {
      p = ST(buffer);
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

static int checkLicence(CSOUND *csound, FILE *unf)
{
    char  *p, *licence;
    int   len = 20;

    csoundMessage(csound, Str("**** Licence Information ****\n"));
    licence = (char*) malloc(len);
    licence[0] = '\0';
    while (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf) != NULL) {
      p = ST(buffer);
      if (strstr(p, "</CsLicence>") != NULL) {
        csoundMessage(csound, Str("**** End of Licence Information ****\n"));
        csoundDestroyGlobalVariable(csound, "::SF::csd_licence");
        csoundCreateGlobalVariable(csound, "::SF::csd_licence", len);
        p = (char*) csoundQueryGlobalVariable(csound, "::SF::csd_licence");
        strcpy(p, licence);
        free(licence);
        return TRUE;
      }
      csoundMessage(csound, "%s", p);
      len += strlen(p);
      licence = realloc(licence, len);
      strcat(licence, p);
    }
    free(licence);
    return FALSE;
}

static int eat_to_eol(char *buf)
{
    int i=0;
    while(buf[i] != '\n') i++;
    return i;   /* keep the \n for further processing */
}

static int blank_buffer(CSOUND *csound)
{
    int i=0;
    if (ST(buffer)[i] == ';')
      i += eat_to_eol(&ST(buffer)[i]);
    while (ST(buffer)[i] != '\n' && ST(buffer)[i] != '\0') {
      if (ST(buffer)[i] != ' ' && ST(buffer)[i] != '\t') return FALSE;
      i++;
    }
    return TRUE;
}

int read_unified_file(CSOUND *csound, char **pname, char **score)
{
    char  *name = *pname;
    FILE  *unf;
    void  *fd;
    int   result = TRUE;
    int   started = FALSE;
    int   r;

    /* Need to open in binary to deal with MIDI and the like. */
    fd = csoundFileOpen(csound, &unf, CSFILE_STD, name, "rb", NULL);
    /* RWD 3:2000 fopen can fail... */
    if (fd == NULL) {
      csound->ErrorMsg(csound, Str("Failed to open csd file: %s"),
                               strerror(errno));
      return 0;
    }
    alloc_globals(csound);
    ST(orcname)[0] = ST(sconame)[0] = ST(midname)[0] = '\0';
    ST(midiSet) = FALSE;
#ifdef _DEBUG
    csoundMessage(csound, "Calling unified file system with %s\n", name);
#endif
    while (my_fgets(ST(buffer), CSD_MAX_LINE_LEN, unf)) {
      char *p = ST(buffer);
      while (*p==' '||*p=='\t') p++;
      if (strstr(p,"<CsoundSynthesizer>") == ST(buffer) ||
          strstr(p,"<CsoundSynthesiser>") == ST(buffer)) {
        csoundMessage(csound, Str("STARTING FILE\n"));
        started = TRUE;
      }
      else if (strstr(p,"</CsoundSynthesizer>") == ST(buffer) ||
               strstr(p,"</CsoundSynthesiser>") == ST(buffer)) {
        *pname = ST(orcname);
        *score = ST(sconame);
        if (ST(midiSet)) {
          csound->oparms->FMidiname = ST(midname);
          csound->oparms->FMidiin = 1;
        }
        csoundFileClose(csound, fd);
        return result;
      }
      else if (strstr(p,"<CsOptions>") == ST(buffer)) {
        csoundMessage(csound, Str("Creating options\n"));
        csound->orchname = NULL;    /* allow orchestra/score name in CSD file */
        r = readOptions(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsInstruments>") == ST(buffer)) {
        csoundMessage(csound, Str("Creating orchestra\n"));
        r = createOrchestra(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsScore>") == ST(buffer)) {
        csoundMessage(csound, Str("Creating score\n"));
        r = createScore(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsMidifile>") == ST(buffer)) {
        r = createMIDI(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsMidifileB>") == ST(buffer)) {
        r = createMIDI2(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsSampleB filename=") == ST(buffer)) {
        r = createSample(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsFileB filename=") == ST(buffer)) {
        r = createFile(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsVersion>") == ST(buffer)) {
        r = checkVersion(csound, unf);
        result = r && result;
      }
      else if (strstr(p,"<CsLicence>") == ST(buffer)) {
        r = checkLicence(csound, unf);
        result = r && result;
      }
      else if (blank_buffer(csound)) continue;
      else if (started && strchr(p, '<') == ST(buffer)) {
        csoundMessage(csound, Str("unknown command: %s\n"), ST(buffer));
      }
    }
    *pname = ST(orcname);
    *score = ST(sconame);
    if (ST(midiSet)) {
      csound->oparms->FMidiname = ST(midname);
      csound->oparms->FMidiin = 1;
    }
    csoundFileClose(csound, fd);
    return result;
}

