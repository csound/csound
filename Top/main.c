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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <ctype.h>
#include "csoundCore.h"         /*                      MAIN.C          */
#include "soundio.h"
#include "csmodule.h"
#include "corfile.h"

#include "csound_orc.h"

#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "cs_par_dispatch.h"

#if defined(USE_OPENMP)
#include <omp.h>
#endif

extern int UDPServerStart(CSOUND *csound, int port);
extern  void    dieu(CSOUND *, char *, ...);
extern  int     argdecode(CSOUND *, int, char **);
extern  int     init_pvsys(CSOUND *);
//extern  char    *get_sconame(CSOUND *);
extern  void    print_benchmark_info(CSOUND *, const char *);
extern  void    openMIDIout(CSOUND *);
extern  int     read_unified_file(CSOUND *, char **, char **);
extern  int     read_unified_file2(CSOUND *csound, char *csd);
extern  uintptr_t  kperfThread(void * cs);
extern void cs_init_math_constants_macros(CSOUND *csound, PRE_PARM *yyscanner);
extern void cs_init_omacros(CSOUND *csound, PRE_PARM*, NAMES *nn);
extern void csoundInputMessageInternal(CSOUND *csound, const char *message);

static void checkOptions(CSOUND *csound)
{
    const char  *csrcname;
    const char  *home_dir;
    FILE        *csrc = NULL;
    void        *fd = NULL;
    char *s;
    /* IV - Feb 17 2005 */
    csrcname = csoundGetEnv(csound, "CSOUND6RC");
    if (csrcname != NULL && csrcname[0] != '\0') {
      fd = csound->FileOpen2(csound, &csrc, CSFILE_STD, csrcname, "r", NULL,
                             CSFTYPE_OPTIONS, 0);
      if (fd == NULL)
        csoundMessage(csound, Str("WARNING: cannot open csound6rc file %s\n"),
                      csrcname);
      else
        csound->Message(csound, Str("Reading options from $CSOUND6RC: %s \n"),
                        csrcname);
    }
    if (fd == NULL && ((home_dir = csoundGetEnv(csound, "HOME")) != NULL &&
                       home_dir[0] != '\0')) {
      s = csoundConcatenatePaths(csound, home_dir, ".csound6rc");
      fd = csound->FileOpen2(csound, &csrc, CSFILE_STD, s, "r", NULL,
                             CSFTYPE_OPTIONS, 0);
      if (fd != NULL)
        csound->Message(csound, Str("Reading options from $HOME/.csound6rc\n"));
      csound->Free(csound, s);
    }
    /* read global .csound6rc file (if exists) */
    if (fd != NULL) {
      readOptions(csound, csrc, 0);
      csound->FileClose(csound, fd);
    }
    /* check for .csound6rc in current directory */
    fd = csound->FileOpen2(csound, &csrc, CSFILE_STD, ".csound6rc", "r", NULL,
                           CSFTYPE_OPTIONS, 0);
    if (fd != NULL) {
      readOptions(csound, csrc, 0);
      csound->Message(csound,
                      Str("Reading options from local directory .csound6rc \n"));
      csound->FileClose(csound, fd);
    }
}


PUBLIC int csoundCompileArgs(CSOUND *csound, int argc, char **argv)
{
    OPARMS  *O = csound->oparms;
    char    *s;
    FILE    *xfile = NULL;
    int     n;
    int     csdFound = 0;
    char    *fileDir;


    if ((n = setjmp(csound->exitjmp)) != 0) {
      return ((n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }

    if(csound->engineStatus & CS_STATE_COMP){
      csound->Message(csound, Str("Csound is already started, call csoundReset()\n"
                                  "before starting again \n"));
       return CSOUND_ERROR;
    }

    if (--argc <= 0) {
      dieu(csound, Str("insufficient arguments"));
    }
    /* command line: allow orc/sco/csd name */
    csound->orcname_mode = 0;   /* 0: normal, 1: ignore, 2: fail */
    if (argdecode(csound, argc, argv) == 0)
      csound->LongJmp(csound, 1);
    /* do not allow orc/sco/csd name in .csound6rc */
    csound->orcname_mode = 2;
    checkOptions(csound);
    if (csound->delayederrormessages) {
      if (O->msglevel>8)
        csound->Warning(csound, csound->delayederrormessages);
      free(csound->delayederrormessages);
      csound->delayederrormessages = NULL;
    }
    /* check for CSD file */
    if (csound->orchname == NULL) {
      if(csound->info_message_request) {
        csound->info_message_request = 0;
        csound->LongJmp(csound, 1);
      }
      else if(csound->oparms->daemon == 0)
         dieu(csound, Str("no orchestra name"));

    }
    else if (csound->use_only_orchfile == 0
             && (csound->scorename == NULL || csound->scorename[0] == (char) 0)
             && csound->orchname[0] != '\0') {
      /* FIXME: allow orc/sco/csd name in CSD file: does this work ? */
      csound->orcname_mode = 0;
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

      if(csound->orchname != NULL) {
      csound->csdname = csound->orchname; /* save original CSD name */
      if (!read_unified_file(csound, &(csound->orchname),
                                       &(csound->scorename))) {
        csound->Die(csound, Str("Reading CSD failed ... stopping"));
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
    if (csound->stdin_assign_flg &&
        (csound->stdin_assign_flg & (csound->stdin_assign_flg - 1)) != 0) {
      csound->Die(csound, Str("error: multiple uses of stdin"));
    }
    if (csound->stdout_assign_flg &&
        (csound->stdout_assign_flg & (csound->stdout_assign_flg - 1)) != 0) {
      csound->Die(csound, Str("error: multiple uses of stdout"));
    }
    /* done parsing csound6rc, CSD, and command line options */

    if (csound->scorename == NULL && csound->scorestr==NULL) {
      /* No scorename yet */
      csound->scorestr = corfile_create_r("f0 800000000000.0\n");
      corfile_flush(csound->scorestr);
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
      csound->orchstr = copy_to_corefile(csound, csound->orchname, NULL, 0);
      if (csound->orchstr==NULL)
        csound->Die(csound,
                    Str("Failed to open input file - %s\n"), csound->orchname);
      corfile_puts("\n#exit\n", csound->orchstr);
      corfile_putc('\0', csound->orchstr);
      corfile_putc('\0', csound->orchstr);
      //csound->orchname = NULL;
    }
    if (csound->xfilename != NULL)
      csound->Message(csound, "xfilename: %s\n", csound->xfilename);

    csoundLoadExternals(csound);    /* load plugin opcodes */
     /* VL: added this also to csoundReset() in csound.c   */
    if (csoundInitModules(csound) != 0)
      csound->LongJmp(csound, 1);
     if(csoundCompileOrc(csound, NULL) != 0){
       if(csound->oparms->daemon == 0)
         csoundDie(csound, Str("cannot compile orchestra"));
       else {
         /* VL -- 21-10-13 Csound does not need to die on
          failure to compile. It can carry on, because new
          instruments can be compiled again */
       csound->Warning(csound, Str("cannot compile orchestra."));
       csound->Warning(csound, Str("Csound will start with no instruments"));
       }
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
    if (!csoundYield(csound))
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
        if (csound->scorestr==NULL)
          csoundDie(csound, Str("cannot open scorefile %s"), csound->scorename);
      }
      csound->Message(csound, Str("sorting score ...\n"));
      scsortstr(csound, csound->scorestr);
      if (csound->keep_tmp) {
        FILE *ff = fopen("score.srt", "w");
        fputs(corfile_body(csound->scstr), ff);
        fclose(ff);
      }
    }
    if (csound->xfilename != NULL) {            /* optionally extract */
      if (!(xfile = fopen(csound->xfilename, "r")))
        csoundDie(csound, Str("cannot open extract file %s"),csound->xfilename);
      csoundNotifyFileOpened(csound, csound->xfilename,
                             CSFTYPE_EXTRACT_PARMS, 0, 0);
      csound->Message(csound, Str("  ... extracting ...\n"));
      scxtract(csound, csound->scstr, xfile);
      fclose(xfile);
      csound->tempStatus &= ~csPlayScoMask;
    }
    csound->Message(csound, Str("\t... done\n"));
    /* copy sorted score name */
    O->playscore = csound->scstr;
    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of score sort"));
    if (O->syntaxCheckOnly) {
      csound->Message(csound, Str("Syntax check completed.\n"));
      return CSOUND_EXITJMP_SUCCESS;
    }

    /* open MIDI output (moved here from argdecode) */
    if (O->Midioutname != NULL && O->Midioutname[0] == (char) '\0')
      O->Midioutname = NULL;
    if (O->FMidioutname != NULL && O->FMidioutname[0] == (char) '\0')
      O->FMidioutname = NULL;
    if (O->Midioutname != NULL || O->FMidioutname != NULL)
      openMIDIout(csound);

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
    if(csound->csdname == NULL)
          checkOptions(csound);

   if(csound->engineStatus & CS_STATE_COMP){
       csound->Message(csound, "Csound is already started, call csoundReset()\n"
                                "before starting again \n");
       return CSOUND_ERROR;
    }

   { /* test for dummy module request */
    char *s;
     if((s = csoundQueryGlobalVariable(csound, "_RTAUDIO")) != NULL)
       if(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
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
  if(csound->enableHostImplementedMIDIIO == 0){
  if((s = csoundQueryGlobalVariable(csound, "_RTMIDI")) != NULL)
    if(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
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
   if(csound->modules_loaded == 0){
    csoundLoadExternals(csound);    /* load plugin opcodes */
    if (csoundInitModules(csound) != 0)
           csound->LongJmp(csound, 1);
    csound->modules_loaded = 1;
    }
    if (csound->instr0 == NULL) { /* compile dummy instr0 to allow csound to
                                     start with no orchestra */
        csoundCompileOrc(csound, "idummy = 0 \n");
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


    if (O->numThreads > 1) {
      void csp_barrier_alloc(CSOUND *, pthread_barrier_t **, int);
      int i;
      THREADINFO *current = NULL;

      csound->multiThreadedBarrier1 = csound->CreateBarrier(O->numThreads);
      csound->multiThreadedBarrier2 = csound->CreateBarrier(O->numThreads);

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
    csound->engineStatus |= CS_STATE_COMP;
    if(csound->oparms->daemon > 1)
        UDPServerStart(csound,csound->oparms->daemon);


    return musmon(csound);
}

PUBLIC int csoundCompile(CSOUND *csound, int argc, char **argv){

  int result = csoundCompileArgs(csound,argc,argv);

  if(result == CSOUND_SUCCESS) return csoundStart(csound);
  else return result;
}



PUBLIC int csoundCompileCsd(CSOUND *csound, char *str) {

  if((csound->engineStatus & CS_STATE_COMP) == 0) {
    char *argv[2] = { "csound", (char *) str };
    int argc = 2;
    return csoundCompile(csound, argc, argv);
  }
  else {
    int res = read_unified_file2(csound, (char *) str);
   if(res) {
    res = csoundCompileOrc(csound, NULL);
    if(res == CSOUND_SUCCESS){
      csoundLockMutex(csound->API_lock);
      char *sc = scsortstr(csound, csound->scorestr);
      csoundInputMessageInternal(csound, (const char *) sc);
      free(sc);
      csoundUnlockMutex(csound->API_lock);
      return CSOUND_SUCCESS;
    }
   }
   return res;
  }
}
