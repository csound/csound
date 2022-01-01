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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include <stdlib.h>
int mkstemp(char *);
#include <ctype.h>
#ifndef __wasi__
#include <errno.h>
#endif
#include <stdlib.h>
#include "corfile.h"

#if defined(LINUX) || defined(__MACH__) || defined(WIN32)
#  include <sys/types.h>
#  include <sys/stat.h>
#endif

#define CSD_MAX_LINE_LEN    4096
#define CSD_MAX_ARGS        100

#  undef L_tmpnam
#  define L_tmpnam (200)

#ifndef TRUE
#  define TRUE  (1)
#endif
#ifndef FALSE
#  define FALSE (0)
#endif

//#define _DEBUG

/* These are used to set/clear bits in csound->tempStatus.
   If the bit is set, it indicates that the given file is
   a temporary. */
const uint32_t csOrcMask     = 1;
const uint32_t csScoInMask   = 2;
const uint32_t csScoSortMask = 4;
const uint32_t csMidiScoMask = 8;
const uint32_t csPlayScoMask = 16;

#define STA(x)   (csound->onefileStatics.x)

CS_NOINLINE char *csoundTmpFileName(CSOUND *csound, const char *ext)
{
#define   nBytes (256)
    char lbuf[256];
#if defined(WIN32) && !defined(__CYGWIN__)
    struct _stat tmp;
#else
    struct stat tmp;
#endif
    do {
#ifndef WIN32
      int fd;
      char *tmpdir = getenv("TMPDIR");
      if (tmpdir != NULL && tmpdir[0] != '\0')
        snprintf(lbuf, nBytes, "%s/csound-XXXXXX", tmpdir);
      else
        strcpy(lbuf, "/tmp/csond-XXXXXX");
      umask(0077);
        /* ensure exclusive access on buggy implementations of mkstemp */
      if (UNLIKELY((fd = mkstemp(lbuf)) < 0))
        csound->Die(csound, Str(" *** cannot create temporary file"));
      close(fd);
      //unlink(lbuf);
#else
      {
        char  *s = (char*) csoundGetEnv(csound, "SFDIR");
        if (s == NULL)
          s = (char*) csoundGetEnv(csound, "HOME");
        s = _tempnam(s, "cs");
        if (UNLIKELY(s == NULL))
          csound->Die(csound, Str(" *** cannot create temporary file"));
        strNcpy(lbuf, s, nBytes);
        free(s);
      }
#endif
      if (ext != NULL && ext[0] != (char) 0) {
#if !defined(LINUX) && !defined(__MACH__) && !defined(WIN32)
        char  *p;
        /* remove original extension (does not work on OS X */
        /* and may be a bad idea) */
        if ((p = strrchr(lbuf, '.')) != NULL)
          *p = '\0';
#endif
        strlcat(lbuf, ext, nBytes);
      }
#ifdef __MACH__
      /* on MacOS X, store temporary files in /tmp instead of /var/tmp */
      /* (suggested by Matt Ingalls) */
      if (strncmp(lbuf, "/var/tmp/", 9) == 0) {
        int i = 3;
        do {
          i++;
          lbuf[i - 4] = lbuf[i];
          } while (lbuf[i] != '\0');
      }
#endif
#if defined(WIN32)
    } while (_stat(lbuf, &tmp) == 0);
#else
      /* if the file already exists, try again */
    } while (stat(lbuf, &tmp) == 0);
#endif
return cs_strdup(csound, lbuf);
}

static inline void alloc_globals(CSOUND *csound)
{
    /* count lines from 0 so that it adds OK to orc/sco counts */
    STA(csdlinecount) = 0;
}

static char *my_fgets(CSOUND *csound, char *s, int n, FILE *stream)
{
    char *a = s;
    if (UNLIKELY(n <= 1)) return NULL;        /* best of a bad deal */
    do {
      int ch = getc(stream);
      if (UNLIKELY(ch == EOF)) {             /* error or EOF       */
        if (s == a) return NULL;             /* no chars -> leave  */
        if (ferror(stream)) a = NULL;
        break; /* add NULL even if ferror(), spec says 'indeterminate' */
      }
      if (ch == '\n' || ch == '\r') {   /* end of line ? */
        ++(STA(csdlinecount));          /* count the lines */
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
    while (STA(toremove) != NULL) {
      NAMELST *nxt = STA(toremove)->next;
#ifdef BETA
      csoundMessage(csound, Str("Removing temporary file %s ...\n"),
                            STA(toremove)->name);
#endif
      if (UNLIKELY(remove(STA(toremove)->name)))
        csoundMessage(csound, Str("WARNING: could not remove %s\n"),
                              STA(toremove)->name);
      csound->Free(csound, STA(toremove)->name);
      csound->Free(csound, STA(toremove));
      STA(toremove) = nxt;
    }
}

void add_tmpfile(CSOUND *csound, char *name)    /* IV - Feb 03 2005 */
{                               /* add temporary file to delete list */
    NAMELST *tmp;
    alloc_globals(csound);
    tmp = (NAMELST*) csound->Malloc(csound, sizeof(NAMELST));
    tmp->name = (char*) csound->Malloc(csound, strlen(name) + 1);
    strcpy(tmp->name, name);
    tmp->next = STA(toremove);
    STA(toremove) = tmp;
}

static int blank_buffer(/*CSOUND *csound,*/ char *buffer)
{
    const char *s;
    for (s = &(buffer[0]); *s != '\0' && *s != '\n'; s++) {
      if (*s == ';')
        return TRUE;
      if (!isblank(*s))
        return FALSE;
    }
    return TRUE;
}


/* Consider wrapping corfile_fgets for this function */
static char *my_fgets_cf(CSOUND *csound, char *s, int n, CORFIL *stream)
{
    char *a = s;
    if (UNLIKELY(n <= 1)) return NULL;        /* best of a bad deal */
    do {
      int ch = corfile_getc(stream);
      if (UNLIKELY(ch == EOF)) {             /* error or EOF       */
        if (s == a) return NULL;             /* no chars -> leave  */
        break;
      }
      if (ch == '\n' || ch == '\r') {   /* end of line ? */
        ++(STA(csdlinecount));          /* count the lines */
        *(s++) = '\n';                  /* convert */
        if (ch == '\r') {
          ch = corfile_getc(stream);
          if (ch != '\n')               /* Mac format */
            corfile_ungetc(stream);
        }
        break;
      }
      *(s++) = ch;
    } while (--n > 1);
    *s = '\0';
    return a;
}

/* readingCsOptions should be non-zero when readOptions() is called
   while reading the <CsOptions> tag, but zero in other cases. */
int readOptions(CSOUND *csound, CORFIL *cf, int readingCsOptions)
{
    char  *p;
    int   argc = 0;
    const char  *argv[CSD_MAX_ARGS];
    char  buffer[CSD_MAX_LINE_LEN];

    //alloc_globals(csound);
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf) != NULL) {
      p = buffer;
      /* Remove trailing spaces; rather heavy handed */
      {
        int len = strlen(p)-2;
        while (len>0 && (isblank(p[len]))) len--;
        p[len+1] = '\n'; p[len+2] = '\0';
      }
      while (isblank(*p)) p++;
      if (readingCsOptions && strstr(p, "</CsOptions>") == p) {
        return TRUE;
      }
      /**
       * Allow command options in unified CSD files
       * to begin with the Csound command, so that
       * the command line arguments can be exactly the same in unified files
       * as for regular command line invocation.
       */
      if (*p==';' || *p=='#' || *p=='\n' || (*p=='/' && *(p+1)=='/'))
        continue; /* empty or comment line? */
      argc = 0;
      argv[0] = p;
      while (isblank(*p)) p++;  /* Ignore leading space */
      if (*p=='-') {        /* Deal with case where no command name is given */
        argv[0] = "csound";
        argv[1] = p;
        argc++;
      }
      while (*p != '\0') {
        if (isblank(*p) && *(p-1)!='\\') {
          *p++ = '\0';
#ifdef _DEBUG
          csoundMessage(csound, "argc=%d argv[%d]=%s\n",
                                argc, argc, argv[argc]);
#endif
          while (isblank(*p)) p++;

          if (*p== '"') {
            if (UNLIKELY(argc == CSD_MAX_ARGS))
              csoundDie(csound, Str("More than %d arguments in <CsOptions>"),
                        CSD_MAX_ARGS);
            argv[++argc] = ++p;
            while (*p != '"' && *p != '\0') {
              if (*p == '\\' && *(p+1) != '\0')
                p++;
              p++;
            }
            if (*p == '"') {
              /* ETX char used to mark the limits of a string */
              *p = (isspace(*(p+1)) ? '\0' : 3);
            }
            //            break;
          }

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
              /* p += 2; */ break;
            }
            if (*p=='*') {
              p++; goto top;
            }
            my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf);
            p = buffer; goto top;
          }
          if (UNLIKELY(argc == CSD_MAX_ARGS))
            csoundDie(csound, Str("More than %d arguments in <CsOptions>"),
                      CSD_MAX_ARGS);
          argv[++argc] = p;
        }
        else if (*p=='\n') {
          *p = '\0';
          break;
        }
        else if (*p=='"' && *(p-1) != '\\') {
          int is_escape = 0;
          char *old = NULL;
          *p=3; /* ETX char used to mark the limits of a string */
          while ((*p != '"' || is_escape) && *p != '\0') {
            if (is_escape)
              *old = 0x18; /* CAN char used to mark a removable character */
            is_escape = (*p == '\\' ? !is_escape : 0);
            old = p;
            p++;
          }
          if (*p == '"') {
            if (isspace(*(p+1))) {
              *p = '\0';
              break;
            }
            else {
              *p = 3;
            }
          }
        }
        p++;
      }
      //argc++;                /* according to Nicola but wrong */
#ifdef _DEBUG
      {
        int i;
        for (i=0;  i<=argc; i++) printf("%d: %s\n", i, argv[i]);
      }
#endif
      /* Read an argv thing */
      if (UNLIKELY(argc == 0)) {
        if (readingCsOptions)
          csoundErrorMsg(csound, Str("Invalid arguments in <CsOptions>: %s"),
                         buffer);
        else csoundErrorMsg(csound,
                         Str("Invalid arguments in .csound6rc or -@ file: %s"),
                         buffer);
      }
      else argdecode(csound, argc, argv);
    }
    if (UNLIKELY(readingCsOptions))
      csoundErrorMsg(csound, Str("Missing end tag </CsOptions>"));
    return FALSE;
}



#if 1
static int all_blank(char* start, char* end)
{
    while (start != end) {
      if (!isblank(*start)) return 0;
      start++;
    }
    return 1;
}

static int createOrchestra(CSOUND *csound, CORFIL *cf)
{
    char  *p, *q;
    CORFIL *incore = corfile_create_w(csound);
    char  buffer[CSD_MAX_LINE_LEN];
    int state = 0;

    csound->orcLineOffset = STA(csdlinecount)+1;
 nxt:
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
      int c;
      p = buffer;

      if (state == 0 &&
          (q = strstr(p, "</CsInstruments>")) &&
          all_blank(buffer,q)) {
        if(csound->oparms->odebug)
          csound->Message(csound, "closing tag\n");
        //corfile_flush(incore);
        corfile_puts(csound, "\n#exit\n", incore);
        corfile_putc(csound, '\0', incore);
        corfile_putc(csound, '\0', incore);
        csound->orchstr = incore;
        return TRUE;
      }
    top:
      if (*p == '\0') continue;
      if (state==0) {
        while ((c = *p++)) {
          if (c=='"') { corfile_putc(csound, c,incore); state = 1; goto top;}
          else if (c=='/' && *p=='*') {
            corfile_putc(csound, c,incore);
            corfile_putc(csound, *p++,incore); state = 2; goto top;
          }
          else if (c == ';'|| (c=='/' && *p=='/')) {
            corfile_puts(csound, p-1, incore); goto nxt;
          }
          else if (c=='{' && *p=='{') {
            corfile_putc(csound, c,incore);
            corfile_putc(csound, *p++,incore); state = 3; goto top;
          }
          corfile_putc(csound, c, incore);
        }
      }
      else if (state == 1) {    /* string */
        while (((c=*p++))) {
          corfile_putc(csound, c, incore);
          if (c=='\\') {
            corfile_putc(csound, *p++, incore);
            if (*p=='\0') goto top;
          }
          else if (c=='"') { state =  0; goto top;}
        }
        csoundErrorMsg(csound, Str("missing \" to terminate string"));
        corfile_rm(csound, &incore);
        return FALSE;
      }
      else if (state == 2) {    /* multiline comment */
        while ( (c = *p++)) {
          if (c=='*' && *p=='/') {
            corfile_putc(csound, c,incore);
            corfile_putc(csound, *p++,incore); state = 0; goto top;
          }
          corfile_putc(csound, c, incore);
        }
        goto nxt;
      }
      else if (state == 3) {    /* extended string */
        while ( (c = *p++)) {
          if (c=='}' && *p=='}') {
            corfile_putc(csound, c,incore); corfile_putc(csound, *p++,incore);
            state = 0; goto top;
          }
          corfile_putc(csound, c, incore);
        }
        goto nxt;
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsInstruments>"));
    corfile_rm(csound, &incore);
    return FALSE;
}
#else
static int createOrchestra(CSOUND *csound, CORFIL *cf)
{
    char  *p;
    CORFIL *incore = corfile_create_w(csound);
    char  buffer[CSD_MAX_LINE_LEN];
    int comm = 0;

    csound->orcLineOffset = STA(csdlinecount)+1;
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
      p = buffer;
      while (isblank(*p)) p++;
      if(*p == '/' && *(p+1) == '*') {
        //csound->Message(csound, "comment start\n");
        comm = 1; p += 2;
      }

      if (comm == 0 &&
          strstr(p, "</CsInstruments>") == p) {
        //csound->Message(csound, "closing tag\n");
        //corfile_flush(incore);
        corfile_puts(csound, "\n#exit\n", incore);
        corfile_putc(csound, '\0', incore);
        corfile_putc(csound, '\0', incore);
        csound->orchstr = incore;
        return TRUE;
      } else if (comm) {
        while (p < buffer + CSD_MAX_LINE_LEN){
          if(*p == '*' && *(p+1) == '/') {
           comm = 0;
           // csound->Message(csound, "comment end\n");
           break;
          } else p++;
        }
        corfile_puts(csound, buffer, incore);
      }
      else
        corfile_puts(csound, buffer, incore);
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsInstruments>"));
    corfile_rm(csound, &incore);
    return FALSE;
}
#endif

#if 1
static int createScore(CSOUND *csound, CORFIL *cf)
{
    char   *p, *q;
    int    state = 0;
    char   buffer[CSD_MAX_LINE_LEN];

    if (csound->scorestr == NULL)
      csound->scorestr = corfile_create_w(csound);
    corfile_putc(csound, '\n', csound->scorestr);
    csound->scoLineOffset = STA(csdlinecount);
 nxt:
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
      int c;
      p = buffer;
      if (state == 0 &&
          (q = strstr(p, "</CsScore>")) &&
          all_blank(buffer,q)) {
        corfile_puts(csound, "\ne\n#exit\n", csound->scorestr);
        corfile_putc(csound, '\0', csound->scorestr); /* For use in bison/flex */
        corfile_putc(csound, '\0', csound->scorestr); /* For use in bison/flex */
        return TRUE;
      }
    top:
      if (*p == '\0') continue;
      if (state==0) {
        while ((c = *p++)) {
          if (c=='"') {
            corfile_putc(csound, c,csound->scorestr); state = 1; goto top;}
          else if (c=='/' && *p=='*') {
            corfile_putc(csound, c,csound->scorestr);
            corfile_putc(csound, *p++,csound->scorestr);
            state = 2; goto top;
          }
          else if (c == ';'|| (c=='/' && *p=='/')) {
            corfile_puts(csound, p-1, csound->scorestr); goto nxt;
          }
          corfile_putc(csound, c, csound->scorestr);
        }
      }
      else if (state == 1) {    /* string */
        while (((c=*p++))) {
          corfile_putc(csound, c, csound->scorestr);
          if (c=='\\') {
            corfile_putc(csound, *p++, csound->scorestr);
            if (*p=='\0') goto top;
          }
          else if (c=='"') { state =  0; goto top;}
        }
        csoundErrorMsg(csound, Str("missing \" to terminate string"));
        corfile_rm(csound, &csound->scorestr);
        return FALSE;
      }
      else if (state == 2) {    /* multiline comment */
        while ( (c = *p++)) {
          if (c=='*' && *p=='/') {
            corfile_putc(csound, c,csound->scorestr);
            corfile_putc(csound, *p++,csound->scorestr);
            state = 0; goto top;
          }
          corfile_putc(csound, c, csound->scorestr);
        }
        goto nxt;
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsScore>"));
    return FALSE;
}
#else
static int createScore(CSOUND *csound, CORFIL *cf)
{
    char   *p;
    char   buffer[CSD_MAX_LINE_LEN];

    if (csound->scorestr == NULL)
      csound->scorestr = corfile_create_w(csound);
    csound->scoLineOffset = STA(csdlinecount);
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
      p = buffer;
      while (isblank(*p)) p++;
      if (strstr(p, "</CsScore>") == p) {
        //#ifdef SCORE_PARSER
        corfile_puts(csound, "\n#exit\n", csound->scorestr);
        corfile_putc(csound, '\0', csound->scorestr); /* For use in bison/flex */
        corfile_putc(csound, '\0', csound->scorestr); /* For use in bison/flex */
        //#endif
        return TRUE;
      }
      else
        corfile_puts(csound, buffer, csound->scorestr);
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsScore>"));
    return FALSE;
}
#endif

static int createExScore(CSOUND *csound, char *p, CORFIL *cf)
{
#ifdef IOS
  csoundErrorMsg(csound, "External scores not supported on iOS");
  return FALSE;
#else
    char *extname;
    char *q;
    char prog[256];
    void *fd;
    FILE  *scof;
    char  buffer[CSD_MAX_LINE_LEN];

    p = strstr(p, "bin=\"");
    if (UNLIKELY(p==NULL)) {
      csoundErrorMsg(csound, Str("Missing program in tag <CsScore>"));
      return FALSE;
    }
    q = strchr(p+5, '"');
    if (UNLIKELY(q==NULL)) {              /* No program given */
      csoundErrorMsg(csound, Str("Missing program in tag <CsScore>"));
      return FALSE;
    }
    *q = '\0';
    strNcpy(prog, p+5, 256); //prog[255]='\0';/* after "<CsExScore " */
    /* Generate score name */
    if (STA(sconame)) free(STA(sconame));
    STA(sconame) = csoundTmpFileName(csound, ".sco");
    extname = csoundTmpFileName(csound, ".ext");
    fd = csoundFileOpenWithType(csound, &scof, CSFILE_STD, extname, "w", NULL,
                                CSFTYPE_SCORE, 1);
    csound->tempStatus |= csScoInMask;
#ifdef _DEBUG
    csoundMessage(csound, Str("Creating %s (%p)\n"), extname, scof);
#endif
    if (UNLIKELY(fd == NULL)) {
      csound->Free(csound, extname);
      return FALSE;
    }

    csound->scoLineOffset = STA(csdlinecount);
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
      p = buffer;
      if (strstr(p, "</CsScore>") == p) {
        char sys[1024];
        csoundFileClose(csound, fd);
        snprintf(sys, 1024, "%s %s %s", prog, extname, STA(sconame));
        if (UNLIKELY(system(sys) != 0)) {
          csoundErrorMsg(csound, Str("External generation failed"));
          if (UNLIKELY(remove(extname) || remove(STA(sconame))))
            csoundErrorMsg(csound, Str("and cannot remove"));
          csound->Free(csound, extname);
          return FALSE;
        }
       if (UNLIKELY(remove(extname)))
         csoundErrorMsg(csound, Str("and cannot remove %s"), extname);
        if (csound->scorestr == NULL)
          csound->scorestr = corfile_create_w(csound);

        fd = csoundFileOpenWithType(csound, &scof, CSFILE_STD, STA(sconame),
                                    "r", NULL, CSFTYPE_SCORE, 0);
        if (UNLIKELY(fd == NULL)) {
          csoundErrorMsg(csound, Str("cannot open %s"), STA(sconame));
          if (UNLIKELY(remove(STA(sconame))))
            csoundErrorMsg(csound, Str("and cannot remove %s"), STA(sconame));
          csound->Free(csound, extname);
          return FALSE;
        }
        csoundMessage(csound, Str("opened %s\n"), STA(sconame));
        while (my_fgets(csound, buffer, CSD_MAX_LINE_LEN, scof)!= NULL)
          corfile_puts(csound, buffer, csound->scorestr);
        csoundMessage(csound, Str("closing %s\n"), STA(sconame));
        csoundFileClose(csound, fd);
        if (UNLIKELY(remove(STA(sconame))))
          csoundErrorMsg(csound, Str("and cannot remove %s\n"), STA(sconame));
        corfile_puts(csound, "\n#exit\n", csound->scorestr);
        corfile_putc(csound, '\0', csound->scorestr);
        corfile_putc(csound, '\0', csound->scorestr);
        //corfile_rewind(csound->scorestr); /* necessary? */
        csound->Free(csound, extname); //27363
        return TRUE;
      }
      else fputs(buffer, scof);
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsScore>"));
    csound->Free(csound, extname);
    return FALSE;
#endif
}

static void read_base64(CSOUND *csound, CORFIL *in, FILE *out)
{
    int c;
    int n, nbits;

    n = nbits = 0;
    while ((c = corfile_getc(in)) != '=' && c != '<') {
      while (isspace(c)) {
        if (c == '\n') {               /* count lines */
          ++(STA(csdlinecount));
          c = corfile_getc(in);
        }
        else if (c == '\r') {
          ++(STA(csdlinecount));
          c = corfile_getc(in);
          if (c == '\n') c = corfile_getc(in); /* DOS format */
        }
        else c = corfile_getc(in);
      }
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
      corfile_ungetc(in);
    if (nbits >= 8) {
      nbits -= 8;
      c = (n >> nbits) & 0xFF;
      n &= ((1 << nbits) - 1);
      putc(c, out);
    }
    if (UNLIKELY(nbits > 0 && n != 0)) {
      csoundDie(csound, Str("Truncated byte at end of base64 stream"));
    }
}
#ifdef JPFF
static void read_base64_2cor(CSOUND *csound, CORFIL *in, CORFIL *out)
{
    int c;
    int n, nbits;

    n = nbits = 0;
    while ((c = corfile_getc(in)) != '=' && c != '<') {
      while (isspace(c)) {
        if (c == '\n') {               /* count lines */
          ++(STA(csdlinecount));
          c = corfile_getc(in);
        }
        else if (c == '\r') {
          ++(STA(csdlinecount));
          c = corfile_getc(in);
          if (c == '\n') c = corfile_getc(in); /* DOS format */
        }
        else c = corfile_getc(in);
      }
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
        corfile_putc(csound, c, out);
      }
    }
    if (c == '<')
      corfile_ungetc(in);
    if (nbits >= 8) {
      nbits -= 8;
      c = (n >> nbits) & 0xFF;
      n &= ((1 << nbits) - 1);
      corfile_putc(csound, c, out);
    }
    if (UNLIKELY(nbits > 0 && n != 0)) {
      csoundDie(csound, Str("Truncated byte at end of base64 stream"));
    }
}
#endif

static int createMIDI2(CSOUND *csound, CORFIL *cf)
{
    char  *p;
    FILE  *midf;
    void  *fd;
    char  buffer[CSD_MAX_LINE_LEN];

    /* Generate MIDI file name */
    if (STA(midname)) free(STA(midname));
    STA(midname) = csoundTmpFileName(csound, ".mid");
    fd = csoundFileOpenWithType(csound, &midf, CSFILE_STD, STA(midname),
                                "wb", NULL, CSFTYPE_STD_MIDI, 1);
    if (UNLIKELY(fd == NULL)) {
      csoundDie(csound, Str("Cannot open temporary file (%s) for MIDI subfile"),
                        STA(midname));
    }
    csound->tempStatus |= csMidiScoMask;
    read_base64(csound, cf, midf);
    csoundFileClose(csound, fd);
    add_tmpfile(csound, STA(midname));               /* IV - Feb 03 2005 */
    STA(midiSet) = TRUE;
    while (TRUE) {
      if (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
        p = buffer;
        while (isblank(*p)) p++;
        if (strstr(p, "</CsMidifileB>") == p) {
          return TRUE;
        }
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsMidifileB>"));
    return FALSE;
}

static int createSample(CSOUND *csound, char *buffer, CORFIL *cf)
{
    int   num;
    FILE  *smpf;
    void  *fd;
    char  sampname[256];
    /* char  buffer[CSD_MAX_LINE_LEN]; */

    sscanf(buffer, "<CsSampleB filename=\"%d\">", &num);
    snprintf(sampname, 256, "soundin.%d", num);
    if (UNLIKELY((smpf = fopen(sampname, "rb")) != NULL)) {
      fclose(smpf);
      csoundDie(csound, Str("File %s already exists"), sampname);
    }
    fd = csoundFileOpenWithType(csound, &smpf, CSFILE_STD, sampname, "wb", NULL,
                                CSFTYPE_UNKNOWN_AUDIO, 1);
    if (UNLIKELY(fd == NULL)) {
      csoundDie(csound, Str("Cannot open sample file (%s) subfile"), sampname);
    }
    read_base64(csound, cf, smpf);
    csoundFileClose(csound, fd);
    add_tmpfile(csound, sampname);              /* IV - Feb 03 2005 */
    while (TRUE) {
      if (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
        char *p = buffer;
        while (isblank(*p)) p++;
        if (strstr(p, "</CsSampleB>") == p) {
          return TRUE;
        }
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsSampleB>"));
    return FALSE;
}

static int createFile(CSOUND *csound, char *buffer, CORFIL *cf)
{
    FILE  *smpf;
    void  *fd;
    char  filename[256];
    char *p = buffer, *q;

    filename[0] = '\0';

    p += 18;    /* 18== strlen("<CsFileB filename=  ") */
    if (*p=='"') {
      p++; q = strchr(p, '"');
    }
    else
      q = strchr(p, '>');
    if (q) *q='\0';
    //  printf("p=>>%s<<\n", p);
    strNcpy(filename, p, 256); //filename[255]='\0';
//sscanf(buffer, "<CsFileB filename=\"%s\">", filename);
//    if (filename[0] != '\0' &&
//       filename[strlen(filename) - 1] == '>' &&
//       filename[strlen(filename) - 2] == '"')
//    filename[strlen(filename) - 2] = '\0';
    if (UNLIKELY((smpf = fopen(filename, "rb")) != NULL)) {
      fclose(smpf);
      csoundDie(csound, Str("File %s already exists"), filename);
    }
    fd = csoundFileOpenWithType(csound, &smpf, CSFILE_STD, filename, "wb", NULL,
                                CSFTYPE_UNKNOWN, 1);
    if (UNLIKELY(fd == NULL)) {
      csoundDie(csound, Str("Cannot open file (%s) subfile"), filename);
    }
    read_base64(csound, cf, smpf);
    csoundFileClose(csound, fd);
    add_tmpfile(csound, filename);              /* IV - Feb 03 2005 */

    while (TRUE) {
      if (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
        char *p = buffer;
        while (isblank(*p)) p++;
        if (strstr(p, "</CsFileB>") == p) {
          return TRUE;
        }
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsFileB>"));
    return FALSE;
}

#ifdef JPFF
static int createCorfile(CSOUND *csound, char *buffer, CORFIL *cf)
{
    CORFIL  *smpf;
    char  filename[256];
    char *p = buffer, *q;

    filename[0] = '\0';

    p += 18;    /* 18== strlen("<CsFileC filename=  ") */
    if (*p=='"') {
      p++; q = strchr(p, '"');
    }
    else
      q = strchr(p, '>');
    if (q) *q='\0';
    //  printf("p=>>%s<<\n", p);
    strNcpy(filename, p, 256); //filename[255]='\0';
//sscanf(buffer, "<CsFileB filename=\"%s\">", filename);
//    if (filename[0] != '\0' &&
//       filename[strlen(filename) - 1] == '>' &&
//       filename[strlen(filename) - 2] == '"')
//    filename[strlen(filename) - 2] = '\0';
    smpf = corfile_create_w(csound);
    read_base64_2cor(csound, cf, smpf);
    corfile_rewind(smpf);
    add_corfile(csound, smpf, filename);

    while (TRUE) {
      if (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)!= NULL) {
        char *p = buffer;
        while (isblank(*p)) p++;
        if (strstr(p, "</CsFileC>") == p) {
          return TRUE;
        }
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsFileC>"));
    return FALSE;
}
#endif

static int createFilea(CSOUND *csound, char *buffer, CORFIL *cf)
{
    FILE  *smpf;
    void  *fd;
    char  filename[256];
    char  buff[1024];
    char *p = buffer, *q;
    int res=FALSE;

    filename[0] = '\0';

    p += 17;    /* 17== strlen("<CsFile filename=  ") */
    if (*p=='"') {
      p++; q = strchr(p, '"');
    }
    else
      q = strchr(p, '>');
    if (q) *q='\0';
    //  printf("p=>>%s<<\n", p);
    strNcpy(filename, p, 256); //filename[255]='\0';
    if (UNLIKELY((smpf = fopen(filename, "r")) != NULL)) {
      fclose(smpf);
      csoundDie(csound, Str("File %s already exists"), filename);
    }
    fd = csoundFileOpenWithType(csound, &smpf, CSFILE_STD, filename, "w", NULL,
                                CSFTYPE_UNKNOWN, 1);
    if (UNLIKELY(fd == NULL)) {
      csoundDie(csound, Str("Cannot open file (%s) subfile"), filename);
    }
    while (corfile_fgets(buff, 1024, cf)!=NULL) {
      char *p = buff;
      while (isblank(*p)) p++;
      if (!strncmp(p, "</CsFile>", 9)) { /* stop on antitag at start of line */
        res = TRUE; break;
      }
      fputs(buff, smpf);
    }
    if (UNLIKELY(res==FALSE))
      csoundErrorMsg(csound, Str("Missing end tag </CsFile>"));
    csoundFileClose(csound, fd);
    add_tmpfile(csound, filename);              /* IV - Feb 03 2005 */
    return res;
}

static int checkVersion(CSOUND *csound, CORFIL *cf)
{
    char  *p;
    int   major = 0, minor = 0;
    int   result = TRUE;
    int   version = csoundGetVersion();
    char  buffer[CSD_MAX_LINE_LEN];

    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf) != NULL) {
      p = buffer;
      while (isblank(*p)) p++;
      if (strstr(p, "</CsVersion>") != NULL)
        return result;
      if (strstr(p, "Before") != NULL) {
        sscanf(p, "Before %d.%d", &major, &minor);
        if (UNLIKELY(version >= ((major * 1000) + (minor*10)))) {
          csoundDie(csound, Str("This CSD file requires a version of "
                                 "Csound before %d.%02d"), major, minor);
          result = FALSE;
        }
      }
      else if (strstr(p, "After") != NULL) {
        sscanf(p, "After %d.%d", &major, &minor);
        if (UNLIKELY(version <= ((major * 1000) + (minor*10)))) {
          csoundDie(csound, Str("This CSD file requires a version of "
                                 "Csound after %d.%02d"), major, minor);
          result = FALSE;
        }
      }
      else if (strstr(p, "Later") != NULL) {
        sscanf(p, "Later %d.%d", &major, &minor);
        if (UNLIKELY(version < ((major * 1000) + (minor*10)))) {
          csoundDie(csound, Str("This CSD file requires version "
                                 "Csound %d.%02d or later"), major, minor);
          result = FALSE;
        }
      }
      else if (sscanf(p, "%d.%d", &major, &minor) == 2) {
        if (UNLIKELY(version <= ((major * 1000) + (minor*10)))) {
          csoundDie(csound, Str("This CSD file requires version "
                                "%d.%02d of Csound"), major, minor);
          result = FALSE;
        }
      }
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsVersion>"));
    return FALSE;
}

static int checkLicence(CSOUND *csound, CORFIL *cf)
{
    char  *p, *licence;
    int   len = 1;
    char  buffer[CSD_MAX_LINE_LEN];

    csoundMessage(csound, Str("**** Licence Information ****\n"));
    licence = (char*) csound->Calloc(csound, len);
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf) != NULL) {
      p = buffer;
      if (strstr(p, "</CsLicence>") != NULL ||
          strstr(p, "</CsLicense>") != NULL) {
        csoundMessage(csound, Str("**** End of Licence Information ****\n"));
        csound->Free(csound, csound->SF_csd_licence);
        csound->SF_csd_licence = licence;
        return TRUE;
      }
      csoundMessage(csound, "%s", p);
      len += strlen(p);
      licence = csound->ReAlloc(csound, licence, len);
      strlcat(licence, p, len);
    }
    csound->Free(csound, licence);
    csoundErrorMsg(csound, Str("Missing end tag </CsLicence>"));
    return FALSE;
}

static int checkShortLicence(CSOUND *csound, CORFIL *cf)
{
    int   type = 0;
    char  buff[CSD_MAX_LINE_LEN];

    csoundMessage(csound, Str("**** Licence Information ****\n"));
    while (my_fgets_cf(csound, buff, CSD_MAX_LINE_LEN, cf) != NULL) {
      if (strstr(buff, "</CsShortLicence>") != NULL ||
          strstr(buff, "</CsShortLicense>") != NULL) {
        csound->SF_id_scopyright = type;
        return TRUE;
      }
      type = atoi(buff);
    }
    csoundErrorMsg(csound, Str("Missing end tag </CsShortLicence>"));
    return FALSE;
}

int read_unified_file4(CSOUND *csound, CORFIL *cf)
{
    int   result = TRUE;
    int   r;
    int started = FALSE;
    int notrunning = csound->engineStatus & CS_STATE_COMP;
    char    buffer[CSD_MAX_LINE_LEN];
#ifdef _DEBUG
    //csoundMessage(csound, "Calling unified file system4\n");
#endif
    if (notrunning==0) {
      alloc_globals(csound);
      STA(orcname) = STA(sconame) = STA(midname) = NULL;
      STA(midiSet) = FALSE;
    }
    while (my_fgets_cf(csound, buffer, CSD_MAX_LINE_LEN, cf)) {
      char *p = buffer;
      while (isblank(*p)) p++;
      if (strstr(p, "<CsoundSynthesizer>") == p ||
          strstr(p, "<CsoundSynthesiser>") == p) {
        if(csound->oparms->odebug)
          csoundMessage(csound, Str("STARTING FILE\n"));
        started = TRUE;
      }
      else if (strstr(p, "</CsoundSynthesizer>") == p ||
               strstr(p, "</CsoundSynthesiser>") == p) {
        if (csound->scorestr != NULL)
          corfile_flush(csound, csound->scorestr);
        corfile_rm(csound, &cf);
        if (notrunning && STA(midiSet)) {
          csound->oparms->FMidiname = STA(midname);
          csound->oparms->FMidiin = 1;
        }
        return result;
      }
      else if (strstr(p, "<CsOptions>") == p) {
        if (!notrunning) {
          if(csound->oparms->odebug)
          csoundMessage(csound, Str("Creating options\n"));
          csound->orchname = NULL;  /* allow orchestra/score name in CSD file */
          r = readOptions(csound, cf, 1);
          result = r && result;
        }
        else {
          csoundMessage(csound, Str("Skipping <CsOptions>\n"));
          do {
            if (UNLIKELY(my_fgets_cf(csound, buffer,
                                   CSD_MAX_LINE_LEN, cf) == NULL)) {
              csoundErrorMsg(csound, Str("Missing end tag </CsOptions>"));
              result = FALSE;
              break;
            }
            p = buffer;
            while (isblank(*p)) p++;
          } while (strstr(p, "</CsOptions>") != p);
        }
      }
      else if (strstr(p, "<CsInstruments>") == p) {
        if(csound->oparms->odebug)
         csoundMessage(csound, Str("Creating orchestra\n"));
        r = createOrchestra(csound, cf);
        result = r && result;
      }
      else if (strstr(p, "<CsScore") == p) {
        if(csound->oparms->odebug)
         csoundMessage(csound, Str("Creating score\n"));
        if (strstr(p, "<CsScore>") == p)
          r = createScore(csound, cf);
        else
          r = createExScore(csound, p, cf);
        result = r && result;
      }
      else if (strstr(p, "<CsMidifileB>") == p) {
        if (notrunning) {
          r = createMIDI2(csound, cf);
          result = r && result;
        }
        else {
          csoundMessage(csound, Str("Skipping <CsMidifileB>\n"));
          do {
            if (UNLIKELY(my_fgets_cf(csound, buffer,
                                   CSD_MAX_LINE_LEN, cf) == NULL)) {
              csoundErrorMsg(csound, Str("Missing end tag </CsMidiFileB>"));
              result = FALSE;
              break;
            }
            p = buffer;
            while (isblank(*p)) p++;
          } while (strstr(p, "</CsMidiFileB>") != p);
        }
      }
      else if (strstr(p, "<CsSampleB filename=") == p) {
        r = createSample(csound, buffer, cf);
        result = r && result;
      }
      else if (strstr(p, "<CsFileB filename=") == p) {
        r = createFile(csound, buffer, cf);
        result = r && result;
      }
#ifdef JPFF
      else if (strstr(p, "<CsFileC filename=") == p) {
        r = createCorfile(csound, buffer, cf);
        result = r && result;
      }
#endif
      else if (strstr(p, "<CsFile filename=") == p) {
        csoundMessage(csound,
                      Str("CsFile is deprecated and may not work; use CsFileB\n"));
        r = createFilea(csound, buffer, cf);
        result = r && result;
      }
      else if (strstr(p, "<CsVersion>") == p) {
        r = checkVersion(csound, cf);
        result = r && result;
      }
      else if (strstr(p, "<CsLicence>") == p ||
               strstr(p, "<CsLicense>") == p) {
        r = checkLicence(csound, cf);
        result = r && result;
      }
      else if (strstr(p, "<CsShortLicence>") == p ||
               strstr(p, "<CsSortLicense>") == p) {
        r = checkShortLicence(csound, cf);
        result = r && result;
      }
      else if (blank_buffer(/*csound,*/ buffer)) continue;
      else if (started && strchr(p, '<') == buffer){
        csoundMessage(csound, Str("unknown CSD tag: %s\n"), buffer);
      }
    }
    if (UNLIKELY(!started)) {
      csoundMessage(csound,
                    Str("Could not find <CsoundSynthesizer> tag in CSD file.\n"));
      result = FALSE;
    }
    corfile_rm(csound, &cf);
    return result;
}
