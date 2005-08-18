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

#include "csoundCore.h"         /*                      MAIN.C          */
#include "soundio.h"
#include "csmodule.h"
#include <ctype.h>              /* For isdigit */

extern  void    dieu(CSOUND *, char *, ...);
extern  int     argdecode(CSOUND *, int, char **);
extern  int     init_pvsys(CSOUND *);
extern  char    *get_sconame(CSOUND *);
extern  int     musmon2(CSOUND *);
extern  char    *getstrformat(int);
extern  void    print_benchmark_info(CSOUND *, const char *);
extern  void    openMIDIout(CSOUND *);

extern  OENTRY  opcodlst_1[];
extern  OENTRY  opcodlst_2[];

static void create_opcodlst(CSOUND *csound)
{
    OENTRY  *saved_opcodlst = NULL;
    int     old_cnt = 0, err = 0;

    if (csound->opcodlst != NULL && csound->oplstend != NULL)
      old_cnt = (int) ((OENTRY*) csound->oplstend - (OENTRY*) csound->opcodlst);
    if (old_cnt) {
      size_t  nBytes = (size_t) ((int) sizeof(OENTRY) * old_cnt);
      saved_opcodlst = (OENTRY*) mmalloc(csound, nBytes);
      memcpy(saved_opcodlst, csound->opcodlst, nBytes);
      free(csound->opcodlst);
      csound->oplstend = csound->opcodlst = NULL;
    }
    /* Basic Entry1 stuff */
    err |= csoundAppendOpcodes(csound, &(opcodlst_1[0]), -1);
    /* Add entry2 */
    err |= csoundAppendOpcodes(csound, &(opcodlst_2[0]), -1);
    /* Add opcodes registered by host application */
    if (old_cnt) {
      err |= csoundAppendOpcodes(csound, saved_opcodlst, old_cnt);
      mfree(csound, saved_opcodlst);
    }
    if (err)
      csoundDie(csound, Str("Error allocating opcode list"));
}

PUBLIC int csoundCompile(CSOUND *csound, int argc, char **argv)
{
    OPARMS  *O = csound->oparms;
    char    *s, *orcNameMode;
    char    *filnamp;
    char    *sortedscore = NULL;
    char    *xtractedscore = "score.xtr";
    char    *playscore = NULL;      /* unless we extract */
    FILE    *scorin = NULL, *scorout = NULL, *xfile = NULL;
    int     n;

    /* IV - Feb 05 2005: find out if csoundPreCompile() needs to be called */
    if (csoundQueryGlobalVariable(csound, "_RTAUDIO") == NULL ||
        csoundQueryGlobalVariable(csound, "csRtClock") != NULL) {
      if ((n = csoundPreCompile(csound)) != CSOUND_SUCCESS)
        return n;
    }

    if ((n = setjmp(csound->exitjmp)) != 0) {
      return ((n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }

    /* IV - Jan 28 2005 */
    csoundCreateGlobalVariable(csound, "csRtClock", sizeof(RTCLOCK));
    init_pvsys(csound);
    /* utilities depend on this as well as orchs */
    dbfs_init(csound, DFLT_DBFS);       /* may get changed by an orch */
    timers_struct_init((RTCLOCK*)
                       csoundQueryGlobalVariable(csound, "csRtClock"));
    csoundCreateGlobalVariable(csound, "#CLEANUP", (size_t) 1);
#ifndef USE_DOUBLE
#ifdef BETA
    csound->Message(csound, Str("Csound version %s beta (float samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#else
    csound->Message(csound, Str("Csound version %s (float samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#endif
#else
#ifdef BETA
    csound->Message(csound, Str("Csound version %s beta (double samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#else
    csound->Message(csound, Str("Csound version %s (double samples) %s\n"),
                            CS_PACKAGE_VERSION, __DATE__);
#endif
#endif
    {
      char buffer[128];
      sf_command (NULL, SFC_GET_LIB_VERSION, buffer, 128);
      csound->Message(csound,"%s\n", buffer);
    }

    /* do not know file type yet */
    O->filetyp = -1;
    O->sfheader = 0;
    O->filnamspace = filnamp = mmalloc(csound, (size_t) 1024);
    csound->peakchunks = 1;
    create_opcodlst(csound);

    if (csoundCreateGlobalVariable(csound, "::argdecode::orcNameMode", 8) != 0)
      return -1;
    orcNameMode = (char*) csoundQueryGlobalVariable(csound,
                                                    "::argdecode::orcNameMode");
    if (--argc == 0) {
      dieu(csound, Str("insufficient arguments"));
    }
    /* command line: allow orc/sco/csd name */
    strcpy(orcNameMode, "normal");
    if (argdecode(csound, argc, argv) == 0)
      csound->LongJmp(csound, 1);
    /* do not allow orc/sco/csd name in .csoundrc */
    strcpy(orcNameMode, "fail");
    {
      const char  *csrcname;
      const char  *home_dir;
      FILE        *csrc = NULL;
      void        *fd = NULL;
      /* IV - Feb 17 2005 */
      csrcname = csoundGetEnv(csound, "CSOUNDRC");
      if (csrcname != NULL && csrcname[0] != '\0') {
        fd = csound->FileOpen(csound, &csrc, CSFILE_STD, csrcname, "r", NULL);
        if (fd == NULL)
          csoundMessage(csound, Str("WARNING: cannot open csoundrc file %s\n"),
                                csrcname);
      }
      if (fd == NULL && ((home_dir = csoundGetEnv(csound, "HOME")) != NULL &&
                         home_dir[0] != '\0')) {
        /* 11 bytes for DIRSEP, ".csoundrc" (9 chars), and null character */
        s = (char*) mmalloc(csound, (size_t) strlen(home_dir) + (size_t) 11);
        sprintf(s, "%s%c%s", home_dir, DIRSEP, ".csoundrc");
        fd = csound->FileOpen(csound, &csrc, CSFILE_STD, s, "r", NULL);
        mfree(csound, s);
      }
      /* read global .csoundrc file (if exists) */
      if (fd != NULL) {
        readOptions(csound, csrc);
        csound->FileClose(csound, fd);
      }
      /* check for .csoundrc in current directory */
      fd = csound->FileOpen(csound, &csrc, CSFILE_STD, ".csoundrc", "r", NULL);
      if (fd != NULL) {
        readOptions(csound, csrc);
        csound->FileClose(csound, fd);
      }
    }
    /* check for CSD file */
    if (csound->orchname == NULL)
      dieu(csound, Str("no orchestra name"));
    else if ((strcmp(csound->orchname+strlen(csound->orchname)-4, ".csd")==0 ||
              strcmp(csound->orchname+strlen(csound->orchname)-4, ".CSD")==0) &&
             (csound->scorename==NULL || strlen(csound->scorename)==0)) {
      int   read_unified_file(void*, char **, char **);
      /* FIXME: allow orc/sco/csd name in CSD file: does this work ? */
      strcpy(orcNameMode, "normal");
      csound->Message(csound,"UnifiedCSD:  %s\n", csound->orchname);
      if (!read_unified_file(csound, &(csound->orchname),
                                     &(csound->scorename))) {
        csound->Die(csound, Str("Decode failed....stopping"));
      }
    }
    /* IV - Feb 19 2005: run a second pass of argdecode so that */
    /* command line options override CSD options */
    /* this assumes that argdecode is safe to run multiple times */
    strcpy(orcNameMode, "ignore");
    argdecode(csound, argc, argv);      /* should not fail this time */
    /* some error checking */
    {
      int *nn;
      nn = (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdinassign");
      if (nn != NULL && *nn != 0 && (*nn & (*nn - 1)) != 0) {
        csound->Die(csound, Str("error: multiple uses of stdin"));
      }
      nn =
        (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdoutassign");
      if (nn != NULL && *nn != 0 && (*nn & (*nn - 1)) != 0) {
        csound->Die(csound, Str("error: multiple uses of stdout"));
      }
    }
    /* done parsing csoundrc, CSD, and command line options */
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
        O->filetyp = TYP_WAV;   /* default to WAV if even SFOUTYP is unset */
    }
    /* everything other than a raw sound file has a header */
    O->sfheader = (O->filetyp == TYP_RAW ? 0 : 1);

    if (csound->scorename == NULL || strlen(csound->scorename) == 0) {
      /* No scorename yet */
      char *sconame = get_sconame(csound);
      char *p;
      FILE *scof;
      mytmpnam(csound, sconame);              /* Generate score name */
      if ((p=strchr(sconame, '.')) != NULL) *p='\0'; /* with extention */
      strcat(sconame, ".sco");
      scof = fopen(sconame, "w");
      fprintf(scof, "f0 42000\n");
      fclose(scof);
      csound->scorename = sconame;
      add_tmpfile(csound, sconame);     /* IV - Feb 03 2005 */
    }
    if (O->Linein || O->Midiin || O->FMidiin)
      O->RTevents = 1;
    if (!O->sfheader)
      O->rewrt_hdr = 0;         /* cannot rewrite header of headerless file */
    if (O->sr_override || O->kr_override) {
      if (!O->sr_override || !O->kr_override)
        dieu(csound, Str("srate and krate overrides must occur jointly"));
    }
    if (!O->outformat)                      /* if no audioformat yet  */
      O->outformat = AE_SHORT;              /*  default to short_ints */
    O->sfsampsize = sfsampsize(FORMAT2SF(O->outformat));
    O->informat = O->outformat;     /* informat defaults; */
    O->insampsiz = O->sfsampsize;   /* resettable by readinheader */
    csound->Message(csound,Str("orchname:  %s\n"), csound->orchname);
    if (csound->scorename != NULL)
      csound->Message(csound,Str("scorename: %s\n"), csound->scorename);
    if (csound->xfilename != NULL)
      csound->Message(csound, Str("xfilename: %s\n"), csound->xfilename);
    /* IV - Oct 31 2002: moved orchestra compilation here, so that named */
    /* instrument numbers are known at the score read/sort stage */
    csoundLoadExternals(csound);    /* load plugin opcodes */
    /* IV - Jan 31 2005: initialise external modules */
    if (csoundInitModules(csound) != 0)
      csound->LongJmp(csound, 1);
    otran(csound);                  /* read orcfile, setup desblks & spaces */
    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of orchestra compile"));
    if (!csoundYield(csound)) return (-1);
    /* IV - Oct 31 2002: now we can read and sort the score */
    if (csound->scorename == NULL || csound->scorename[0]=='\0') {
      if (O->RTevents) {
        csound->Message(csound, Str("realtime performance using dummy "
                                    "numeric scorefile\n"));
        goto perf;
      }
      else {
        if (csound->keep_tmp) {
          csound->scorename = "score.srt";
        }
        else {
          char *scnm = csound->Calloc(csound, (size_t) 256);
          csound->scorename = mytmpnam(csound, scnm);
          add_tmpfile(csound, csound->scorename);       /* IV - Feb 03 2005 */
        }
      }
    }
    if ((n = strlen(csound->scorename)) > 4     /* if score ?.srt or ?.xtr */
        && (!strcmp(csound->scorename+n-4,".srt") ||
            !strcmp(csound->scorename+n-4,".xtr"))) {
      csound->Message(csound,Str("using previous %s\n"),csound->scorename);
      playscore = sortedscore = csound->scorename;  /*   use that one */
    }
    else {
      if (csound->keep_tmp) {
        playscore = sortedscore = "score.srt";
      }
      else {
        char *nme = csound->Calloc(csound, (size_t) 256);
        playscore = sortedscore = mytmpnam(csound, nme);
        add_tmpfile(csound, playscore);         /* IV - Feb 03 2005 */
      }
      if (!(scorin = fopen(csound->scorename, "rb")))   /* else sort it   */
        csoundDie(csound, Str("cannot open scorefile %s"), csound->scorename);
      if (!(scorout = fopen(sortedscore, "w")))
        csoundDie(csound, Str("cannot open %s for writing"), sortedscore);
      csound->Message(csound, Str("sorting score ...\n"));
      scsort(csound, scorin, scorout);
      fclose(scorin);
      fclose(scorout);
    }
    if (csound->xfilename != NULL) {            /* optionally extract */
      if (!strcmp(csound->scorename,"score.xtr"))
        csoundDie(csound, Str("cannot extract %s, name conflict"),
                          csound->scorename);
      if (!(xfile = fopen(csound->xfilename, "r")))
        csoundDie(csound, Str("cannot open extract file %s"),csound->xfilename);
      if (!(scorin = fopen(sortedscore, "r")))
        csoundDie(csound, Str("cannot reopen %s"), sortedscore);
      if (!(scorout = fopen(xtractedscore, "w")))
        csoundDie(csound, Str("cannot open %s for writing"), xtractedscore);
      csound->Message(csound,Str("  ... extracting ...\n"));
      scxtract(csound, scorin, scorout, xfile);
      fclose(scorin);
      fclose(scorout);
      fclose(xfile);
      playscore = xtractedscore;
    }
    csound->Message(csound,Str("\t... done\n"));
    s = playscore;
    O->playscore = filnamp;
    while ((*filnamp++ = *s++));    /* copy sorted score name */
    /* IV - Jan 28 2005 */
    print_benchmark_info(csound, Str("end of score sort"));
 perf:
    O->filnamsize = filnamp - O->filnamspace;
    /* open MIDI output (moved here from argdecode) */
    if (O->Midioutname != NULL && O->Midioutname[0] != '\0')
      openMIDIout(csound);

    return musmon(csound);
}

PUBLIC int csoundPerform(CSOUND *csound, int argc, char **argv)
{
    int     n;

    if ((n = setjmp(csound->exitjmp))) {
      csound->Message(csound, "Early return from csoundPerform().\n");
      return (n - CSOUND_EXITJMP_SUCCESS);
    }
    n = csoundCompile(csound, argc, argv);
    csound->Message(csound, "Compile returns %d\n", n);
    if (n)
      return n;
    if ((n = setjmp(csound->exitjmp))) {
      csound->Message(csound, "Early return from csoundPerform().\n");
      return (n - CSOUND_EXITJMP_SUCCESS);
    }
    n = musmon2(csound);
    csound->Message(csound, "musmon returns %d\n", n);
    return n;
}

