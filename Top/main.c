/*
    main.c:
    Copyright (C) 1991-2002 Barry Vercoe, John ffitch
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

#include <ctype.h>
#include "csoundCore.h"         /*                      MAIN.C          */
#include "soundio.h"
#include "csmodule.h"
#include "corfile.h"

#include "csound_orc.h"

#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
//#include "cs_par_dispatch.h"

extern void allocate_message_queue(CSOUND *csound);
CS_NORETURN void    dieu(CSOUND *, char *, ...);
  int     argdecode(CSOUND *, int, const char **);
  int     init_pvsys(CSOUND *);
//  char    *get_sconame(CSOUND *);
  void    print_benchmark_info(CSOUND *, const char *);
//  int     read_unified_file(CSOUND *, char **, char **);
//  int     read_unified_file2(CSOUND *csound, char *csd);
  int     read_unified_file4(CSOUND *csound, CORFIL *csd);
  uintptr_t  kperfThread(void * cs);
//void cs_init_math_constants_macros(CSOUND *csound, PRE_PARM *yyscanner);
//void cs_init_omacros(CSOUND *csound, PRE_PARM*, NAMES *nn);
 void csoundInputMessageInternal(CSOUND *csound, const char *message);
 int csoundCompileOrcInternal(CSOUND *csound, const char *str, int async);

static void checkOptions(CSOUND *csound)
{
    const char  *csrcname;
    const char  *home_dir;
    FILE        *csrc = NULL;
    void        *fd = NULL;
    char        *s = NULL;
    /* IV - Feb 17 2005 */
    csrcname = csoundGetEnv(csound, "CSOUND6RC");
    if (csrcname != NULL && csrcname[0] != '\0') {
      fd = csound->FileOpen2(csound, &csrc, CSFILE_STD, csrcname, "r", NULL,
                             CSFTYPE_OPTIONS, 0);
      if (UNLIKELY(fd == NULL)) {
          csoundMessage(csound, Str("WARNING: cannot open csound6rc file %s\n"),
              csrcname);
      } else {
          csound->Message(csound, Str("Reading options from $CSOUND6RC: %s\n"),
              csrcname);
          s = csound->Strdup(csound, (char*)csrcname);
      }
    }
    if (fd == NULL && ((home_dir = csoundGetEnv(csound, "HOME")) != NULL &&
                       home_dir[0] != '\0')) {
      s = csoundConcatenatePaths(csound, home_dir, ".csound6rc");
      fd = csound->FileOpen2(csound, &csrc, CSFILE_STD, s, "r", NULL,
                             CSFTYPE_OPTIONS, 0);
      if (fd != NULL)
        csound->Message(csound, Str("Reading options from $HOME/.csound6rc\n"));
      //csound->Free(csound, s);
    }
    /* read global .csound6rc file (if exists) */
    if (fd != NULL) {
      CORFIL *cf = copy_to_corefile(csound, s, NULL, 0);
      corfile_rewind(cf);
      readOptions(csound, cf, 0);
      corfile_rm(csound, &cf);
      csound->FileClose(csound, fd);
      csound->Free(csound, s);
    }
    /* check for .csound6rc in current directory */
     fd = csound->FileOpen2(csound, &csrc, CSFILE_STD, ".csound6rc", "r", NULL,
                           CSFTYPE_OPTIONS, 0);
    if (fd != NULL) {
      CORFIL *cf = copy_to_corefile(csound, ".csound6rc", NULL, 0);
      corfile_rewind(cf);
      readOptions(csound, cf, 0);
      csound->Message(csound,
                      Str("Reading options from local directory .csound6rc\n"));
      corfile_rm(csound, &cf);
      csound->FileClose(csound, fd);
    }
}

static void put_sorted_score(CSOUND *csound, char *ss, FILE* ff)
{
    char *p = ss;
    int num, cnt;
    double p2o, p2, p3o,p3, inst;
    while (*p!='\0') {
      switch (*p) {
      case 'f':
        fputc(*p,ff);
        sscanf(p+1, "%d %la %la%n", &num, &p2o, &p2, &cnt);
        fprintf(ff, " %d %lg %lg ", num, p2o, p2);
        p+=cnt+1;
        break;
      case 'i':
      case 'd':
        fputc(*p,ff);
        if (*(p+2)=='"') {
          fputc(' ', ff);
          fputc('"', ff);
          p+=3;
          while (*p!='"') {
            fputc(*p++, ff);
          }
          sscanf(p+1, "%la %la %la %la%n", &p2o, &p2, &p3o, &p3, &cnt);
          fprintf(ff, "\" %lg %lg %lg %lg ", p2o, p2, p3o, p3);
          p+=cnt+1;
        }
        else {
          sscanf(p+1, "%la %la %la %la %la%n", &inst, &p2o, &p2, &p3o, &p3, &cnt);
          fprintf(ff, " %lg %lg %lg %lg %lg ", inst, p2o, p2, p3o, p3);
          p+=cnt+1;
        }
        break;
      default:
        csound->Message(csound, Str("Unknown score opcode %c(%.2x)\n"), *p, *p);
      case 's':
      case 'e':
      case 'w':
        break;
      }
      /* Others could be numbers or strings */
      while (1) {
        //printf("** p '%c'\n", *p);
        if (!strncmp(p, "0x", 2)) {
          sscanf(p, "%la%n", &p3, &cnt);
          fprintf(ff, "%lg ", p3);
          p+=cnt-1;
        }
        else fputc(*p, ff);
        if (*p++=='\n') break;
      }
    }
}

PUBLIC int csoundCompileArgs(CSOUND *csound, int argc, const char **argv)
{
    OPARMS  *O = csound->oparms;
    char    *s;
    FILE    *xfile = NULL;
    int     n;
    volatile int     csdFound = 0;
    volatile int ac = argc;
    char    *fileDir;
    volatile int     compiledOk = 0;

    if ((n = setjmp(csound->exitjmp)) != 0) {
      return ((n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }

    argc = ac;
    if (UNLIKELY(csound->engineStatus & CS_STATE_COMP)) {
      csound->Message(csound, Str("Csound is already started, call csoundReset()\n"
                                  "before starting again.\n"));
      return CSOUND_ERROR;
    }

    if (UNLIKELY(--argc <= 0)) {
      dieu(csound, Str("insufficient arguments"));
    }
    /* command line: allow orc/sco/csd name */
    csound->orcname_mode = 0;   /* 0: normal, 1: ignore, 2: fail */
    if (UNLIKELY(argdecode(csound, argc, argv) == 0))
      csound->LongJmp(csound, 1);
    /* do not allow orc/sco/csd name in .csound6rc */
    csound->orcname_mode = 2;
    checkOptions(csound);
    if (csound->delayederrormessages) {
      if (O->msglevel>8)
        csound->Warning(csound, "%s", csound->delayederrormessages);
      csound->Free(csound, csound->delayederrormessages);
      csound->delayederrormessages = NULL;
    }

    /* check for CSD file */
    if (csound->orchname == NULL) {
      if (csound->info_message_request) {
        csound->info_message_request = 0;
        csound->LongJmp(csound, 1);
      }
      else if (UNLIKELY(csound->oparms->daemon == 0))
         dieu(csound, Str("no orchestra name"));

    }
    else if (csound->use_only_orchfile == 0
             && (csound->scorename == NULL || csound->scorename[0] == (char) 0)
             && csound->orchname[0] != '\0') {
      /* FIXME: allow orc/sco/csd name in CSD file: does this work ? */
      csound->orcname_mode = 0;
      if(O->msglevel || O->odebug)
       csound->Message(csound, "UnifiedCSD:  %s\n", csound->orchname);

      /* Add directory of CSD file to search paths before orchname gets
       * replaced with temp orch name if default paths is enabled */
      if (!O->noDefaultPaths) {
        fileDir = csoundGetDirectoryForPath(csound, csound->orchname);
        csoundAppendEnv(csound, "SADIR", fileDir);
        csoundAppendEnv(csound, "SSDIR", fileDir);
        csoundAppendEnv(csound, "INCDIR", fileDir);
        csoundAppendEnv(csound, "MFDIR", fileDir);
        csound->Free(csound, fileDir);
      }

      if (csound->orchname != NULL) {
        csound->csdname = csound->orchname; /* save original CSD name */
      {
        CORFIL *cf = copy_to_corefile(csound, csound->csdname, NULL, 0);
        if (UNLIKELY(cf == NULL)) {
          csound->Die(csound, Str("Reading CSD failed (%s)... stopping"),
                      strerror(errno));
        }
        corfile_rewind(cf);
        if (UNLIKELY(!read_unified_file4(csound, cf))) {
          csound->Die(csound, Str("Reading CSD failed (%s)... stopping"),
                      strerror(errno));
        }
        /* cf is deleted in read_unified_file4 */
      }
      csdFound = 1;
      }
    }

    /* IV - Feb 19 2005: run a second pass of argdecode so that */
    /* command line options override CSD options */
    /* this assumes that argdecode is safe to run multiple times */
    csound->orcname_mode = 1;           /* ignore orc/sco name */
    argdecode(csound, argc, argv);      /* should not fail this time */
    /* some error checking */
    if (UNLIKELY(csound->stdin_assign_flg &&
         (csound->stdin_assign_flg & (csound->stdin_assign_flg - 1)) != 0)) {
      csound->Die(csound, Str("error: multiple uses of stdin"));
    }
    if (UNLIKELY(csound->stdout_assign_flg &&
       (csound->stdout_assign_flg & (csound->stdout_assign_flg - 1)) != 0)) {
      csound->Die(csound, Str("error: multiple uses of stdout"));
    }
    /* done parsing csound6rc, CSD, and command line options */

    if (csound->scorename == NULL && csound->scorestr==NULL) {
      /* No scorename yet */
      csound->Message(csound, "scoreless operation\n");
      // csound->scorestr = corfile_create_r("f0 800000000000.0\n");
      // VL 21-09-2016: it looks like #exit is needed for the
      // new score parser to work.
      // was "\n#exit\n" but seemed to have zero effect;
      csound->scorestr = corfile_create_r(csound, "\n\n\ne\n#exit\n");
      corfile_flush(csound, csound->scorestr);
      if (O->RTevents)
        csound->Message(csound, Str("realtime performance using dummy "
                                    "numeric scorefile\n"));
    }
    else if (!csdFound && !O->noDefaultPaths){
      /* Add directory of SCO file to search paths*/
      fileDir = csoundGetDirectoryForPath(csound, csound->scorename);
      csoundAppendEnv(csound, "SADIR", fileDir);
      csoundAppendEnv(csound, "SSDIR", fileDir);
      csoundAppendEnv(csound, "MFDIR", fileDir);
      csound->Free(csound, fileDir);
    }

    /* Add directory of ORC file to search paths*/
    if (!csdFound && !O->noDefaultPaths) {
      fileDir = csoundGetDirectoryForPath(csound, csound->orchname);
      csoundAppendEnv(csound, "SADIR", fileDir);
      csoundAppendEnv(csound, "SSDIR", fileDir);
      csoundAppendEnv(csound, "MFDIR", fileDir);
      csound->Free(csound, fileDir);
    }

    if (csound->orchstr==NULL && csound->orchname) {
      /*  does not deal with search paths */
      csound->Message(csound, Str("orchname:  %s\n"), csound->orchname);
      csound->orcLineOffset = 1; /* Guess -- JPff */
      csound->orchstr = copy_to_corefile(csound, csound->orchname, NULL, 0);
      if (UNLIKELY(csound->orchstr==NULL))
        csound->Die(csound,
                    Str("main: failed to open input file - %s\n"), csound->orchname);
      corfile_puts(csound, "\n#exit\n", csound->orchstr);
      corfile_putc(csound, '\0', csound->orchstr);
      corfile_putc(csound, '\0', csound->orchstr);
      corfile_rewind(csound->orchstr);
      //csound->orchname = NULL;
    }
    if (csound->xfilename != NULL)
      csound->Message(csound, "xfilename: %s\n", csound->xfilename);

    csoundLoadExternals(csound);    /* load plugin opcodes */
     /* VL: added this also to csoundReset() in csound.c   */
    if (csoundInitModules(csound) != 0)
      csound->LongJmp(csound, 1);

    if (UNLIKELY(csoundCompileOrcInternal(csound, NULL, 0) != 0)){
      if (csound->oparms->daemon == 0)
        csoundDie(csound, Str("cannot compile orchestra"));
      else {
        /* VL -- 21-10-13 Csound does not need to die on
           failure to compile. It can carry on, because new
           instruments can be compiled again */
        if (csound->oparms->daemon == 0)
          csound->Warning(csound, Str("cannot compile orchestra.\n"
                                      "Csound will start with no instruments"));
       }
    } else {
      compiledOk = 1;
    }
    csound->modules_loaded = 1;

    s = csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (csound->enableHostImplementedMIDIIO == 1) {
        if (s) {
          strcpy(s, "hostbased");
        }
        csoundSetConfigurationVariable(csound,"rtmidi", "hostbased");
    }

    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of orchestra compile"));
    if (UNLIKELY(!csoundYield(csound)))
      return -1;
    /* IV - Oct 31 2002: now we can read and sort the score */

    if (csound->scorename != NULL &&
        (n = strlen(csound->scorename)) > 4 &&  /* if score ?.srt or ?.xtr */
        (!strcmp(csound->scorename + (n - 4), ".srt") ||
         !strcmp(csound->scorename + (n - 4), ".xtr"))) {
      csound->Message(csound, Str("using previous %s\n"), csound->scorename);
      //playscore = sortedscore = csound->scorename;   /*  use that one */
      csound->scorestr = NULL;
      csound->scorestr = copy_to_corefile(csound, csound->scorename, NULL, 1);
    }
    else {
      //sortedscore = NULL;
      if (csound->scorestr==NULL) {
        csound->scorestr = copy_to_corefile(csound, csound->scorename, NULL, 1);
        if (UNLIKELY(csound->scorestr==NULL))
          csoundDie(csound, Str("cannot open scorefile %s"), csound->scorename);
      }
      if(O->msglevel || O->odebug)
       csound->Message(csound, Str("sorting score ...\n"));
      //printf("score:\n%s", corfile_current(csound->scorestr));
      scsortstr(csound, csound->scorestr);
      //printf("*** keep_tmp = %d\n", csound->keep_tmp);
      if (csound->keep_tmp) {
        FILE *ff = fopen("score.srt", "w");
        if (csound->keep_tmp==1)
          fputs(corfile_body(csound->scstr), ff);
        else
          put_sorted_score(csound, corfile_body(csound->scstr), ff);
        fclose(ff);
      }
    }
    if (csound->xfilename != NULL) {            /* optionally extract */
      if (UNLIKELY(!(xfile = fopen(csound->xfilename, "r"))))
        csoundDie(csound, Str("cannot open extract file %s"),csound->xfilename);
      csoundNotifyFileOpened(csound, csound->xfilename,
                             CSFTYPE_EXTRACT_PARMS, 0, 0);
       if(O->msglevel || O->odebug)
         csound->Message(csound, Str("  ... extracting ...\n"));
      scxtract(csound, csound->scstr, xfile);
      fclose(xfile);
      csound->tempStatus &= ~csPlayScoMask;
    }
     if(O->msglevel || O->odebug)
       csound->Message(csound, Str("\t... done\n"));
    /* copy sorted score name */
    O->playscore = csound->scstr;
    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of score sort"));
    if (O->syntaxCheckOnly) {
      csound->Message(csound, Str("Syntax check completed.\n"));
      // return CSOUND_EXITJMP_SUCCESS;
      if (compiledOk)
          return CSOUND_EXITJMP_SUCCESS;
      return CSOUND_ERROR;
    }
    return CSOUND_SUCCESS;
}

extern int  playopen_dummy(CSOUND *, const csRtAudioParams *parm);
extern void rtplay_dummy(CSOUND *, const MYFLT *outBuf, int nbytes);
extern int  recopen_dummy(CSOUND *, const csRtAudioParams *parm);
extern int  rtrecord_dummy(CSOUND *, MYFLT *inBuf, int nbytes);
extern void rtclose_dummy(CSOUND *);
extern int  audio_dev_list_dummy(CSOUND *, CS_AUDIODEVICE *, int);
extern int  midi_dev_list_dummy(CSOUND *csound, CS_MIDIDEVICE *list, int isOutput);
extern int DummyMidiInOpen(CSOUND *csound, void **userData,
                           const char *devName);
extern int DummyMidiRead(CSOUND *csound, void *userData,
                         unsigned char *buf, int nbytes);
extern int DummyMidiOutOpen(CSOUND *csound, void **userData,
                     const char *devName);
extern int DummyMidiWrite(CSOUND *csound, void *userData,
                   const unsigned char *buf, int nbytes);


PUBLIC int csoundStart(CSOUND *csound) // DEBUG
{
    OPARMS  *O = csound->oparms;
    int     n;

    /* if a CSD was not used, check options */
    if (csound->csdname == NULL)
          checkOptions(csound);

    if (UNLIKELY(csound->engineStatus & CS_STATE_COMP)){
      csound->Message(csound, Str("Csound is already started, call csoundReset()\n"
                                  "before starting again.\n"));
      return CSOUND_ERROR;
    }

    { /* test for dummy module request */
      char *s;
      if ((s = csoundQueryGlobalVariable(csound, "_RTAUDIO")) != NULL)
        if (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
            strcmp(s, "NULL") == 0) {
          csound->Message(csound, Str("setting dummy interface\n"));
          csound->SetPlayopenCallback(csound, playopen_dummy);
          csound->SetRecopenCallback(csound, recopen_dummy);
          csound->SetRtplayCallback(csound, rtplay_dummy);
          csound->SetRtrecordCallback(csound, rtrecord_dummy);
          csound->SetRtcloseCallback(csound, rtclose_dummy);
          csound->SetAudioDeviceListCallback(csound, audio_dev_list_dummy);
        }

      /* and midi */
      if (csound->enableHostImplementedMIDIIO == 0){
        if ((s = csoundQueryGlobalVariable(csound, "_RTMIDI")) != NULL)
          if (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
              strcmp(s, "NULL") == 0) {
            csound->SetMIDIDeviceListCallback(csound, midi_dev_list_dummy);
            csound->SetExternalMidiInOpenCallback(csound, DummyMidiInOpen);
            csound->SetExternalMidiReadCallback(csound,  DummyMidiRead);
            csound->SetExternalMidiInCloseCallback(csound, NULL);
            csound->SetExternalMidiOutOpenCallback(csound,  DummyMidiOutOpen);
            csound->SetExternalMidiWriteCallback(csound, DummyMidiWrite);
            csound->SetExternalMidiOutCloseCallback(csound, NULL);
          }
      }
      else {
        s = csoundQueryGlobalVariable(csound, "_RTMIDI");
        if (s)
          strcpy(s, "hostbased");
        csoundSetConfigurationVariable(csound,"rtmidi", "hostbased");
      }
    }


    /* VL 30-12-12 csoundInitModules is always called here now to enable
       Csound to start without calling csoundCompile, but directly from
       csoundCompileOrc() and csoundReadSco()
    */
    if (csound->modules_loaded == 0){
      csoundLoadExternals(csound);    /* load plugin opcodes */
      if (csoundInitModules(csound) != 0)
        csound->LongJmp(csound, 1);
      csound->modules_loaded = 1;
    }
    if (csound->instr0 == NULL) { /* compile dummy instr0 to allow csound to
                                     start with no orchestra */
      csoundCompileOrcInternal(csound, "idummy = 0\n", 0);
    }

    if ((n = setjmp(csound->exitjmp)) != 0) {
      return ((n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }



    /* if sound file type is still not known, check SFOUTYP */
    if (O->filetyp <= 0) {
      const char  *envoutyp;
      envoutyp = csoundGetEnv(csound, "SFOUTYP");
      if (envoutyp != NULL && envoutyp[0] != '\0') {
        if (strcmp(envoutyp, "AIFF") == 0)
          O->filetyp = TYP_AIFF;
        else if (strcmp(envoutyp, "WAV") == 0 || strcmp(envoutyp, "WAVE") == 0)
          O->filetyp = TYP_WAV;
        else if (strcmp(envoutyp, "IRCAM") == 0)
          O->filetyp = TYP_IRCAM;
        else if (strcmp(envoutyp, "RAW") == 0)
          O->filetyp = TYP_RAW;
        else {
          dieu(csound, Str("%s not a recognised SFOUTYP env setting"),
               envoutyp);
        }
      }
      else
#if !defined(__MACH__)
        O->filetyp = TYP_WAV;   /* default to WAV if even SFOUTYP is unset */
#else
        O->filetyp = TYP_AIFF;  /* ... or AIFF on the Mac */
#endif
    }
    /* everything other than a raw sound file has a header */
    O->sfheader = (O->filetyp == TYP_RAW ? 0 : 1);
    if (O->Linein || O->Midiin || O->FMidiin)
      O->RTevents = 1;
    if (!O->sfheader)
      O->rewrt_hdr = 0;         /* cannot rewrite header of headerless file */
    /* VL 9 04 15: these not need occur jointly anymore */
    /*
      if (O->sr_override || O->kr_override) {
      if (!O->sr_override || !O->kr_override)
        dieu(csound, Str("srate and krate overrides must occur jointly"));
        } */
    if (!O->outformat)                      /* if no audioformat yet  */
      O->outformat = AE_SHORT;              /*  default to short_ints */
    O->sfsampsize = sfsampsize(FORMAT2SF(O->outformat));
    O->informat = O->outformat;             /* informat default */

#ifdef PARCS
    if (O->numThreads > 1) {
      void csp_barrier_alloc(CSOUND *, void **, int);
      int i;
      THREADINFO *current = NULL;

      csp_barrier_alloc(csound, &(csound->barrier1), O->numThreads);
      csp_barrier_alloc(csound, &(csound->barrier2), O->numThreads);

      csound->multiThreadedComplete = 0;

      for (i = 1; i < O->numThreads; i++) {
        THREADINFO *t = csound->Malloc(csound, sizeof(THREADINFO));

        t->threadId = csound->CreateThread(&kperfThread, (void *)csound);
        t->next = NULL;

        if (current == NULL) {
          csound->multiThreadedThreadInfo = t;
        }
        else {
          current->next = t;
        }
        current = t;
      }

      csound->WaitBarrier(csound->barrier2);
    }
#endif
    csound->engineStatus |= CS_STATE_COMP;
    if (csound->oparms->daemon > 1)
      csoundUDPServerStart(csound,csound->oparms->daemon);

    allocate_message_queue(csound); /* if de-alloc by reset */
    return musmon(csound);
}

PUBLIC int csoundCompile(CSOUND *csound, int argc, const char **argv){

    int result = csoundCompileArgs(csound,argc,argv);

    if (result == CSOUND_SUCCESS) return csoundStart(csound);
    else return result;
}

PUBLIC int csoundCompileCsd(CSOUND *csound, const char *str) {
    CORFIL *tt = copy_to_corefile(csound, str, NULL, 0);
    if (LIKELY(tt != NULL)) {
      int res = csoundCompileCsdText(csound, tt->body);
      corfile_rm(csound, &tt);
      return res;
    }
    return CSOUND_ERROR;
}

PUBLIC int csoundCompileCsdText(CSOUND *csound, const char *csd_text)
{
    //csound->oparms->odebug = 1; /* *** SWITCH ON EXTRA DEBUGGING *** */
    int res = read_unified_file4(csound, corfile_create_r(csound, csd_text));
    //printf("file read res = %d\n", res);
    if (LIKELY(res)) {
      if (csound->csdname != NULL) csound->Free(csound, csound->csdname);
      csound->csdname = cs_strdup(csound, "*string*"); /* Mark as from text. */
      res = csoundCompileOrcInternal(csound, NULL, 0);
      //printf("internalread res = %d\n", res);
      if (res == CSOUND_SUCCESS){
        if ((csound->engineStatus & CS_STATE_COMP) != 0) {
          /* if (csound->scorestr==NULL) { */
          /*   printf("*** no score\n"); */
          /*   csound->scorestr = corfile_create_w(csound); */
          /*   corfile_puts(csound, "\nf0 800000000000.0\ne\n#exit\n",csound->scorestr); */
          /*   //corfile_puts(csound, "e\n#exit\n",csound->scorestr); */
          /* } */
          {                     /* Ned to exsure tere is no e opcode before '#exit'.  Thiscode is flacky */
            char *sc;
            if (csound->scorestr==NULL)
              sc = "#exit";
            else {
              //printf("INPUT STRING %s\n", csound->scorestr->body+csound->scorestr->len-9);
              csound->scorestr->body[+csound->scorestr->len-9] = ' ';
              //printf("Mangld >>%s<<\n", csound->scorestr->body);
              //corfile_puts(csound, "\n#exit\n", csound->scorestr);
              //printf("*** in score >>%s<<<\n", corfile_body(csound->scorestr));
              sc = scsortstr(csound, csound->scorestr);
            }
            //printf("*** Out score >>>%s<<<\n", sc);
            if (sc) {
              if (csound->oparms->odebug)
                csound->Message(csound,
                                Str("Real-time score events (engineStatus: %d).\n"),
                                csound->engineStatus);
              csoundInputMessage(csound, (const char *) sc);
            }
          }
        } else {
          if (csound->scorestr==NULL) {
            csound->scorestr = corfile_create_w(csound);
            corfile_puts(csound, "\n\n\ne\n#exit\n",csound->scorestr);
          }
          scsortstr(csound, csound->scorestr);
          if (csound->oparms->odebug)
            csound->Message(csound,
                            Str("Compiled score "
                                "(engineStatus: %d).\n"), csound->engineStatus);
        }
      }
      return res;
    }
    else return CSOUND_ERROR;
}
