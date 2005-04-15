/*
  argdecode.c:

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


#include "cs.h"                 /*                              ARG_DECODE.C */
#include "prototyp.h"
#include "soundio.h"
#include "new_opts.h"
#include "csmodule.h"
#include <ctype.h>

#ifdef mills_macintosh
#include <SIOUX.h>
#include "perf.h"
#include"MacTransport.h"

#define PATH_LEN        128

extern struct Transport transport;
extern Boolean displayText;

extern char saved_scorename[];
extern unsigned char mytitle[];
extern Boolean util_perf;
extern unsigned short pollEventRate;

static char listing_file[PATH_LEN];
static int  vbuf;
static int csNGraphs;
static int rescale24 = 0;
#endif

#define FIND(MSG)   if (*s == '\0')  \
                      if (!(--argc) || (((s = *++argv) != NULL) && *s == '-')) \
                         dieu(MSG);

#define STDINASSIGN_SNDFILE     1
#define STDINASSIGN_LINEIN      2
#define STDINASSIGN_MIDIFILE    4
#define STDINASSIGN_MIDIDEV     8

#define STDOUTASSIGN_SNDFILE    1
#define STDOUTASSIGN_MIDIOUT    2

/* IV - Feb 19 2005 */

static void set_stdin_assign(void *csound, int type, int state)
{
  int *n;
  n = (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdinassign");
  if (n == NULL) {
    csoundCreateGlobalVariable(csound,
                               "::argdecode::stdinassign", sizeof(int));
    n = (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdinassign");
  }
  if (state)
    *n |= type;
  else
    *n &= (~type);
}

static void set_stdout_assign(void *csound, int type, int state)
{
  int *n;
  n = (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdoutassign");
  if (n == NULL) {
    csoundCreateGlobalVariable(csound,
                               "::argdecode::stdoutassign", sizeof(int));
    n = (int*) csoundQueryGlobalVariable(csound, "::argdecode::stdoutassign");
  }
  if (state)
    *n |= type;
  else
    *n &= (~type);
}

/* IV - Feb 19 2005 */
static const char *shortUsageList[] = {
  "--help\tprint long usage options",
  "-U unam\trun utility program unam",
  "-C\tuse Cscore processing of scorefile",
  "-I\tI-time only orch run",
  "-n\tno sound onto disk",
  "-i fnam\tsound input filename",
  "-o fnam\tsound output filename",
  "-b N\tsample frames (or -kprds) per software sound I/O buffer",
  "-B N\tsamples per hardware sound I/O buffer",
  "-A\tcreate an AIFF format output soundfile",
  "-W\tcreate a WAV format output soundfile",
  "-J\tcreate an IRCAM format output soundfile",
  "-h\tno header on output soundfile",
  "-c\t8-bit signed_char sound samples",
#ifdef never
  "-a\talaw sound samples",
#endif
  "-8\t8-bit unsigned_char sound samples",
  "-u\tulaw sound samples",
  "-s\tshort_int sound samples",
  "-l\tlong_int sound samples",
  "-f\tfloat sound samples",
  "-3\t24bit sound samples",
  "-r N\torchestra srate override",
  "-k N\torchestra krate override",
  "-K\tDo not generate PEAK chunks",
  "-v\tverbose orch translation",
  "-m N\ttty message level. Sum of:",
  "\t\t1=note amps, 2=out-of-range msg, 4=warnings",
  "\t\t0/32/64/96=note amp format (raw,dB,colors)",
  "\t\t128=print benchmark information",
  "-d\tsuppress all displays",
  "-g\tsuppress graphics, use ascii displays",
  "-G\tsuppress graphics, use Postscript displays",
  "-x fnam\textract from score.srt using extract file 'fnam'",
  "-t N\tuse uninterpreted beats of the score, initially at tempo N",
  "-t 0\tuse score.srt for sorted score rather than a temporary",
  "-L dnam\tread Line-oriented realtime score events from device 'dnam'",
  "-M dnam\tread MIDI realtime events from device 'dnam'",
  "-F fnam\tread MIDIfile event stream from file 'fnam'",
  /*  "-P N\tMIDI sustain pedal threshold (0 - 128)", */
  "-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)",
  "-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write",
  "-N\tnotify (ring the bell) when score or miditrack is done",
  "-T\tterminate the performance when miditrack is done",
  "-D\tdefer GEN01 soundfile loads until performance time",
  "-Q dnam\tselect MIDI output device",
  "-z\tList opcodes in this version",
  "-Z\tDither output",
#if defined(LINUX)
  "--sched     set real-time priority and lock memory",
  "            (requires -d and real time audio (-iadc/-odac))",
  "--sched=N   set specified scheduling priority, and lock memory",
  "            (requires -d and real time audio (-iadc/-odac))",
#endif
#ifdef mills_macintosh
  "_____________Macintosh Command Line Flags_________________",
  "-V N\t Number of chars in screen buffer for output window",
  "-E N\t Number of tables in graphics window",
  "-p\t\t Play after rendering",
  "-e\t\t Rescaled floats as shorts to max amplitude",
  "-w\t\t Record and Save MIDI input to a file",
  "-y N\t Enables Progress Display at rate N seconds,",
  "\t\t\tor for negative N, at -N kperiods",
  "-Y N\t Enables Profile Display at rate N in seconds,",
  "\t\t\tor for negative N, at -N kperiods",
  "-P N\t Poll Events Every N Buffer Writes",
  "__________________________________________________________",
#endif
  NULL
};

/* IV - Feb 19 2005 */
void print_short_usage(void *csound)
{
  ENVIRON *p;
  char    buf[256];
  int     i;
  p = (ENVIRON*) csound;
  i = -1;
  while (shortUsageList[++i] != NULL) {
    sprintf(buf, "%s\n", shortUsageList[i]);
    p->Message(csound, Str(buf));
  }
  p->Message(csound, Str("flag defaults: csound -s -otest -b%d -B%d -m7\n"),
                     IOBUFSAMPS, IODACSAMPS);
}

void usage(void)
{
  ENVIRON *csound = &cenviron;

  csound->Message(csound, Str("Usage:\tcsound [-flags] orchfile scorefile\n"));
  csound->Message(csound, Str("Legal flags are:\n"));
  print_short_usage(csound);
  longjmp(csound->exitjmp, 1);
}

static void longusage(void *csound)
{
    ENVIRON *p = (ENVIRON*)csound;

  p->Message(csound,Str("Usage:\tcsound [-flags] orchfile scorefile\n"));
  p->Message(csound,Str("Legal flags are:\n"));
  p->Message(csound,Str("Long format:\n\n"));
  p->Message(csound,"--format={alaw,ulaw,"
             "schar,uchar,float,short,long,24bit,rescale}\t Set sound type\n"
             "--aiff\t\t\tSet AIFF format\n"
             "--au\t\t\tSet AU format\n"
             "--wave\t\t\tSet WAV format\n"
             "--ircam\t\t\tSet IRCAM format\n"
             "--noheader\t\tRaw format\n"
             "--nopeaks\t\tDo not write peak information\n"
             "\n"
             "--nodisplays\t\tsuppress all displays\n"
             "--asciidisplay\t\tsuppress graphics, use ascii displays\n"
             "--postscriptdisplay\tsuppress graphics, use Postscript displays\n"
             "\n"
             "--defer-gen1\t\tdefer GEN01 soundfile loads until performance "
             "time\n"
             "--iobufsamps=N\t\tsample frames (or -kprds) per software "
             "sound I/O buffer\n"
             "--hardwarebufsamps=N\tsamples per hardware sound I/O buffer\n"
             "--cscore\t\tuse Cscore processing of scorefile\n"
             "\n"
             "--midifile=FNAME\tread MIDIfile event stream from file\n"
             "--midi-device=FNAME\tread MIDI realtime events from device\n"
             "--terminate-on-midi\tterminate the performance when "
             "miditrack is done\n"
             "\n"
             "--heartbeat=N\t\tprint a heartbeat style 1, 2 or 3 at "
             "each soundfile write\n"
             "--notify\t\tnotify (ring the bell) when score or "
             "miditrack is done\n"
             "--rewrite\t\tcontinually rewrite header while writing "
             "soundfile (WAV/AIFF)\n"
             "\n"
             "--input=FNAME\t\tsound input filename\n"
             "--output=FNAME\t\tsound output filename\n"
             "--logfile=FNAME\t\tlog output to file\n"
             "\n"
             "--nosound\t\tno sound onto disk or device\n"
             "--tempo=N\t\tuse uninterpreted beats of the score, "
             "initially at tempo N\n"
             "--i-only\t\tI-time only orch run\n"
             "--control-rate=N\torchestra krate overrid\n"
             "--sample-rate=N\t\torchestra srate override\n"
             "--score-in=FNAME\tread Line-oriented realtime score "
             "events from device\n"
             "--messagelevel=N\ttty message level. Sum of: 1=note amps, "
             "2=out-of-range msg, 4=warnings\n\n"
             "\n"
             "--extract-score=FNAME\textract from score.srt using extract file\n"
             "--keep-sorted-score\n"
             "--expression-opt\toptimise use of temporary variables in expressions\n"
             "--env:NAME=VALUE\tset environment variable NAME to VALUE\n"
             "--env:NAME+=VALUE\tappend VALUE to environment variable NAME\n"
             "--utility=NAME\t\trun utility program\n"
             "--verbose\t\tverbose orch translation\n"
             "--list-opcodes\t\tList opcodes in this version\n"
             "--list-opcodesN\t\tList opcodes in style N in this version\n"
             "--dither\t\tDither output\n"
             "--sched\t\t\tset real-time scheduling priority and lock memory\n"
             "--sched=N\t\tset priority to N and lock memory\n"
             "--opcode-lib=NAMES\tDynamic libraries to load\n"
             "\n"
             "--help\t\t\tLong help\n"
#ifdef mills_macintosh
             "\n--graphs=N\tNumber of tables in graphics window\n"
             "--pollrate=N\n"
             "--play-on-end\t Play after rendering\n"
             "--screen-buffer=N\tNumber of chars in screen buffer for "
             "output window\n"
             "--save-midi\tRecord and Save MIDI input to a file\n"
#endif
             );
  /* IV - Feb 19 2005 */
  dump_cfg_variables(csound);
  p->Message(csound,Str("\nShort form:\n"));
  print_short_usage(csound);
  longjmp(p->exitjmp, CSOUND_EXITJMP_SUCCESS);
}

void dieu(char *s)
{
    ENVIRON *csound = &cenviron;
    csound->Message(csound,Str("Csound Command ERROR:\t%s\n"), s);
    usage();
    longjmp(csound->exitjmp, 1);
}

typedef struct {
  char *util;
  int (*fn)(int, char**);
  char *string;
} UTILS;

UTILS utilities[] = {
  { "hetro",   hetro,   "util HETRO:\n" },
  { "lpanal",  lpanal,  "util LPANAL:\n" },
  { "pvanal",  pvanal,  "util PVANAL:\n" },
  { "sndinfo", sndinfo, "util SNDINFO:\n" },
  { "cvanal",  cvanal,  "util CVANAL:\n" },
  { "pvlook",  pvlook,  "util PVLOOK:\n" },
  { "dnoise",  dnoise,  "util DNOISE:\n" },
  { NULL, NULL, 0}
};

void set_output_format(OPARMS *O, char c)
{
  switch (c) {
  case 'a':
    O->outformat = AE_ALAW;    /* a-law soundfile */
    break;

  case 'c':
    O->outformat = AE_CHAR;    /* signed 8-bit soundfile */
    break;

  case '8':
    O->outformat = AE_UNCH;    /* unsigned 8-bit soundfile */
    break;

  case 'f':
    O->outformat = AE_FLOAT;   /* float soundfile */
    break;

  case 's':
    O->outformat = AE_SHORT;   /* short_int soundfile*/
    break;

  case 'l':
    O->outformat = AE_LONG;    /* long_int soundfile */
    break;

  case 'u':
    O->outformat = AE_ULAW;    /* mu-law soundfile */
    break;

  case '3':
    O->outformat = AE_24INT;   /* 24bit packed soundfile*/
    break;

  case 'e':
    O->outformat = AE_FLOAT;   /* float soundfile (for rescaling) */
    break;

  default:
    return; /* do nothing */
  };
}

typedef struct  {
  char *longformat;
  char shortformat;
} SAMPLE_FORMAT_ENTRY;

static
SAMPLE_FORMAT_ENTRY sample_format_map[] = {
  {"alaw", 'a'},  {"schar", 'c'},{"uchar", '8'},
  {"float", 'f'},
  {"short", 's'}, {"ulaw", 'u'}, {"24bit", '3'},
  {0, 0}
};

static int decode_long(void *csound_, char *s, int argc, char **argv)
{
  ENVIRON *csound = (ENVIRON*) csound_;
  OPARMS  *O = csound->oparms;
  /* Add other long options here */
  if (!(strncmp(s, "format=", 7)))
    {
      SAMPLE_FORMAT_ENTRY *sfe = sample_format_map;
      char c = '\0';

      s += 7;
      while (sfe->longformat != 0)
        {
          if (strcmp(s, sfe->longformat) == 0)
            {
              c = sfe->shortformat;
              break;
            }
          ++sfe;
        }

      if (c != '\0')
        {
          set_output_format(O, c);
          return 1;
        }
#ifdef mills_macintosh
      else if (!(strncmp (s, "rescale", 7))) {
        s += 7;
        set_output_format(O, 'e');
        rescale24 = atoi(s);
        if (rescale24)
          SetRescaleFloatFileTo24(TRUE);
        SetRescaleFloatFile(TRUE);
        return 1;
      }
#endif
    }
  /* -A */
  else if (!(strcmp (s, "aiff"))) {
    O->sfheader = 1;
    O->filetyp = TYP_AIFF;     /* AIFF output request*/
    return 1;
  }
  else if (!(strcmp (s, "au"))) {
    O->sfheader = 1;
    O->filetyp = TYP_AU;       /* AU output request*/
    return 1;
  }
  else if (!(strncmp (s, "iobufsamps=", 11))) {
    s += 11;
    if (*s=='\0') dieu(Str("no iobufsamps"));
    /* defaults in musmon.c */
    O->inbufsamps = O->outbufsamps = atoi(s);
    return 1;
  }
  else if (!(strncmp (s, "hardwarebufsamps=", 17))) {
    s += 17;
    if (*s=='\0') dieu(Str("no hardware bufsamps"));
    O->inbufsamps = O->outbufsamps = atoi(s);
    return 1;
  }
  else if (!(strcmp (s, "cscore"))) {
    O->usingcscore = 1;     /* use cscore processing  */
    return 1;
  }
  else if (!(strcmp (s, "nodisplays"))) {
    O->displays = 0;           /* no func displays */
    return 1;
  }
  else if (!(strcmp (s, "defer-gen1"))) {
    O->gen01defer = 1;  /* defer GEN01 sample loads
                          until performance time */
    return 1;
  }
#ifdef mills_macintosh
  /* -E N */
  else if (!(strncmp (s, "graphs=", 7))) {
    s += 7;
    if (*s=='\0') dieu(Str("no number of graphs"));
    csNGraphs = atoi(s);
    SetCsNGraphs(csNGraphs);
    return 1;
  }
  else if (!(strncmp (s, "pollrate=", 9))) {
    s += 9;
    if (*s == '\0') dieu(Str("no poll event rate"));
    pollEventRate = atoi(s);
    return 1;
  }
  else if (!(strcmp(s, "play-on-end"))) {
    SetPlayOnFinish(TRUE);
    return 1;
  }
#endif
  else if (!(strncmp (s, "midifile=", 9))) {
    s += 9;
    if (*s == '\0') dieu(Str("no midifile name"));
    O->FMidiname = s;            /* Midifile name */
    if (!strcmp(O->FMidiname,"stdin")) {
      set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#if defined(WIN32) || defined(mills_macintosh)
      csoundDie(csound, Str("-F: stdin not supported on this platform"));
#endif
    }
    else
      set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
    O->FMidiin = 1;              /***************/
    return 1;
  }
  /* -g */
  else if (!(strcmp (s, "asciidisplay"))) {
    O->graphsoff = 1;          /* don't use graphics but ASCII */
    return 1;
  }
  /* -G */
  else if (!(strcmp (s, "postscriptdisplay"))) {
    O->postscript = 1;        /* don't use graphics but PostScript */
    return 1;
  }
  /* -h */
  else if (!(strcmp (s, "noheader"))) {
    O->sfheader = 0;          /* skip sfheader  */
    O->filetyp = TYP_RAW;
    return 1;
  }
  else if (!(strncmp (s, "heartbeat=", 10))) {
    s += 10;
    if (*s == '\0') O->heartbeat = 1;
    else O->heartbeat = atoi(s);
    return 1;
  }
#ifdef EMBEDDED_PYTHON
  else if (strncmp(s, "pyvar=", 6) == 0) {
    s += 6;
    if (python_add_cmdline_definition(s))
      dieu(Str("invalid python variable definition syntax"));
    return 1;
  }
#endif
  else if (!(strncmp (s, "input=", 6))) {
    s += 6;
    if (*s == '\0') dieu(Str("no infilename"));
    O->infilename = s;   /* soundin name */
    if (strcmp(O->infilename,"stdout") == 0)
      csoundDie(csound, Str("input cannot be stdout"));
    if (strcmp(O->infilename,"stdin") == 0) {
      set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mills_macintosh)
      csoundDie(csound, Str("stdin audio not supported"));
#endif
    }
    else
      set_stdin_assign(csound, STDINASSIGN_SNDFILE, 0);
    O->sfread = 1;
    return 1;
  }
  /*
    -I I-time only orch run
  */
  else if (!(strcmp (s, "i-only"))) {
    O->initonly = 1;
    return 1;
  }
  /*
    -j Used in localisation
    -J create an IRCAM format output soundfile
  */
  else if (!(strcmp (s, "ircam"))) {
    O->sfheader = 1;
    O->filetyp = TYP_IRCAM;      /* IRCAM output request */
    return 1;
  }
  /*
    -k N orchestra krate override
  */
  else if (!(strncmp(s, "control-rate=", 13))) {
    s += 13;
    if (*s=='\0') dieu(Str("no control rate"));
    O->kr_override = atoi(s);
    return 1;
  }
  /* -K */
  else if (!(strcmp (s, "nopeaks"))) {
    peakchunks = 0;     /* Do not write peak information */
    return 1;
  }
  /*
    -L dnam read Line-oriented realtime score events from device 'dnam'
  */
  else if (!(strncmp (s, "score-in=", 9))) {
    s += 9;
    if (*s=='\0') dieu(Str("no Linein score device_name"));
    O->Linename = s;
    if (!strcmp(O->Linename,"stdin")) {
      set_stdin_assign(csound, STDINASSIGN_LINEIN, 1);
#if defined(mills_macintosh)
      csoundDie(csound, Str("-L: stdin not supported on this platform"));
#endif
    }
    else
      set_stdin_assign(csound, STDINASSIGN_LINEIN, 0);
    O->Linein = 1;
    return 1;
  }
  /*
    -m N tty message level. Sum of: 1=note amps, 2=out-of-range msg, 4=warnings
  */
  else if (!(strncmp (s, "messagelevel=", 13))) {
    s += 13;
    if (*s=='\0') dieu(Str("no message level"));
    O->msglevel = atoi(s);
    return 1;
  }
  /*
    -M dnam read MIDI realtime events from device 'dnam'
  */
  else if (!(strncmp (s, "midi-device=", 12))) {
    s += 12;
    if (*s=='\0') dieu(Str("no midi device_name"));
    O->Midiname = s;
    if (!strcmp(O->Midiname,"stdin")) {
      set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#if defined(WIN32) || defined(mills_macintosh)
      csoundDie(csound, Str("-M: stdin not supported on this platform"));
#endif
    }
    else
      set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 0);
    O->Midiin = 1;
    return 1;
  }
  /* -n no sound */
  else if (!(strcmp (s, "nosound"))) {
    O->sfwrite = 0;            /* nosound        */
    return 1;
  }
  /* -N */
  else if (!(strcmp (s, "notify"))) {
    O->ringbell = 1;        /* notify on completion */
    return 1;
  }
  else if (!(strncmp (s, "output=", 7))) {
    s += 7;
    if (*s == '\0') dieu(Str("no outfilename"));
    O->outfilename = s;          /* soundout name */
    if (strcmp(O->outfilename,"stdin") == 0)
      dieu(Str("-o cannot be stdin"));
    if (strcmp(O->outfilename,"stdout") == 0) {
      set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mills_macintosh)
      csoundDie(csound, Str("stdout audio not supported"));
#endif
    }
    else
      set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 0);
    O->sfwrite = 1;
    return 1;
  }
  else if (!(strncmp (s, "logfile=", 8))) {
    s += 8;
    if (*s=='\0') dieu(Str("no log file"));
    return 1;
  }
  /* -r N */
  else if (!(strncmp (s, "sample-rate=", 12))) {
    s += 12;
    O->sr_override = atol(s);
    return 1;
  }
  /* R */
  else if (!(strcmp (s, "rewrite"))) {
    O->rewrt_hdr = 1;
    return 1;
  }
  /* -S  */
  /* tempo=N use uninterpreted beats of the score, initially at tempo N
   */
  else if (!(strncmp (s, "tempo=", 6))) {
    s += 6;
    O->cmdTempo = atoi(s);
    O->Beatmode = 1;       /* on uninterpreted Beats */
    return 1;
  }
  /* -t0 */
  else if (!(strcmp (s, "keep-sorted-score"))) {
    csound->keep_tmp = 1;
    return 1;
  }
  /* IV - Jan 27 2005: --expression-opt */
  else if (!(strcmp (s, "expression-opt"))) {
    O->expr_opt = 1;
    return 1;
  }
  else if (!(strncmp (s, "env:", 4))) {
    if (csoundParseEnv(csound, s + 4) == CSOUND_SUCCESS)
      return 1;
    else
      return 0;
  }
  /* -T terminate the performance when miditrack is done */
  else if (!(strcmp (s, "terminate-on-midi"))) {
    O->termifend = 1;          /* terminate on midifile end */
    return 1;
  }
  else if (!(strncmp (s, "utility=", 8))) {
    int n, retval;
    s += 8;
    if (*s=='\0') dieu(Str("no utility name"));
#ifdef mills_macintosh
    util_perf = true;
    transport.state |= kUtilPerf;
#endif
    for (n=0; utilities[n].util!=NULL; n++) {
      if (strcmp(s,utilities[n].util) == 0) {
        csound->Message(csound,Str(utilities[n].string));
#ifdef mills_macintosh
        SIOUXSetTitle((unsigned char *)CtoPstr((char *)s));
#endif
        retval = (utilities[n].fn)(argc,argv);
        if (!retval)
          longjmp(csound->exitjmp, CSOUND_EXITJMP_SUCCESS);
        else
          longjmp(csound->exitjmp, abs(retval));
      }
    }
    csoundDie(csound, Str("-U %s not a valid UTIL name"),s);
    return 0;
  }
  /* -v */
  else if (!(strcmp (s, "verbose"))) {
    O->odebug = 1;    /* verbose otran  */
    return 1;
  }
#ifdef mills_macintosh
  /* -V N  Number of chars in screen buffer for output window */
  else if (!(strncmp(s, "screen-buffer=", 14))) {
    s += 14;
    if (*s=='\0') dieu(Str("no screen buffer size"));
    vbuf = atoi(s);
    fflush(stdout);
    setvbuf(stdout, NULL, _IOFBF, vbuf);
    return 1;
  }
  /* -w   Record and Save MIDI input to a file */
  else if ((!strcmp(s, "save-midi")) || (!strcmp(s, "record-midi"))) {
    SetRecordMIDIData(TRUE);
    return 1;
  }
#endif
  /* -x fnam extract from score.srt using extract file 'fnam' */
  else if (!(strncmp(s, "extract-score=", 14))) {
    s += 14;
    if (*s=='\0') dieu(Str("no xfilename"));
    xfilename = s;
    return 1;
  }
  else if (!(strcmp(s, "wave"))) {
    O->sfheader = 1;
    O->filetyp = TYP_WAV;      /* WAV output request */
    return 1;
  }
  else if (!(strcmp(s, "wave64"))) {
    O->sfheader = 1;
    O->filetyp = TYP_W64;      /* WAVE 64 output request */
    return 1;
  }
  else if (!(strcmp(s, "voc"))) {
    O->sfheader = 1;
    O->filetyp = TYP_VOC;      /* VOC output request */
    return 1;
  }
  else if (!(strncmp (s, "list-opcodes", 12))) {
    int full = 0;
    s += 12;

    if (*s != '\0') {
      if (isdigit(*s)) full = *s++ - '0';
    }
    create_opcodlst(csound);
    if (csoundInitModules(csound) != 0)
      longjmp(csound->exitjmp, 1);
    list_opcodes(full);
    longjmp(csound->exitjmp, CSOUND_EXITJMP_SUCCESS);
  }
  /* -Z */
  else if (!(strcmp (s, "dither"))) {
    csound->dither_output = 1;
    return 1;
  }
  else if (!(strncmp(s, "opcode-lib=", 11))) {
    s += 11;
    {
      char  *oplibs;
      int   nbytes;
      nbytes = (int) strlen(s);
      oplibs =
        (char*) csoundQueryGlobalVariable(csound, "::dl_opcodes::oplibs");
      if (oplibs == NULL) {
        csoundCreateGlobalVariable(csound, "::dl_opcodes::oplibs", 4096);
        oplibs =
          (char*) csoundQueryGlobalVariable(csound, "::dl_opcodes::oplibs");
        if (oplibs == NULL) {
          csoundMessage(csound, "argdecode(): memory allocation failure\n");
          return 0;
        }
      }
      else
        nbytes += ((int) strlen(oplibs) + 1);
      if (nbytes >= 4096) {
        csoundMessage(csound, "argdecode(): opcode library list too long\n");
        return 0;
      }
      if (oplibs[0] == '\0') {
        /* start new library list */
        strcpy(oplibs, s);
      }
      else {
        /* append to existing list */
        strcat(oplibs, ",");
        strcat(oplibs, s);
      }
    }
    return 1;
  }
  else if (!(strcmp(s ,"help"))) {
    longusage(csound);
    longjmp(csound->exitjmp, CSOUND_EXITJMP_SUCCESS);
  }

  csoundMessage(csound, Str("unknown long option: '--%s'\n"), s);
  return (0);
}

int argdecode(void *csound_, int argc, char **argv_)
{
  ENVIRON *csound = (ENVIRON*) csound_;
  OPARMS  *O = csound->oparms;
  char    *s, **argv;
  int     n;
  char    c;

  /* make a copy of the option list */
  {
    char  *p1, *p2;
    int   nbytes, i;
    /* calculate the number of bytes to allocate */
    /* N.B. the argc value passed to argdecode is decremented by one */
    nbytes = (argc + 1) * (int) sizeof(char*);
    for (i = 0; i <= argc; i++)
      nbytes += ((int) strlen(argv_[i]) + 1);
    p1 = (char*) mmalloc(csound, nbytes);   /* will be freed by all_free() */
    p2 = (char*) p1 + ((int) sizeof(char*) * (argc + 1));
    argv = (char**) p1;
    for (i = 0; i <= argc; i++) {
      argv[i] = p2;
      strcpy(p2, argv_[i]);
      p2 = (char*) p2 + ((int) strlen(argv_[i]) + 1);
    }
  }
  csound->keep_tmp = 0;

  do {

    s = *++argv;
    if (*s++ == '-') {                        /* read all flags:  */
      while ((c = *s++) != '\0') {
        switch(c) {
        case 'U':
          FIND(Str("no utility name"));
#ifdef mills_macintosh
          util_perf = true;
          transport.state |= kUtilPerf;
#endif
          for (n=0; utilities[n].util!=NULL; n++) {
            if (strcmp(s,utilities[n].util) == 0) {
              int retval;
              csound->Message(csound, Str(utilities[n].string));
#ifdef mills_macintosh
              SIOUXSetTitle((unsigned char *)CtoPstr((char *)s));
#endif
              retval = (utilities[n].fn)(argc,argv);
              if (!retval)
                longjmp(csound->exitjmp, CSOUND_EXITJMP_SUCCESS);
              else
                longjmp(csound->exitjmp, abs(retval));
            }
          }
          csoundDie(csound, Str("-U %s not a valid UTIL name"),s);
          return(0);
          /********** commandline flags only for mac version***************/
          /*********************  matt 5/26/96 ****************************/
#ifdef mills_macintosh
        case 'V':
          FIND(Str("no screen buffer size"));
          sscanf(s,"%d",&vbuf);
          fflush(stdout);
          setvbuf(stdout, NULL, _IOFBF, vbuf);
          while (*++s);
          break;
        case 'P':
          FIND(Str("no poll event rate"));
          sscanf(s,"%d",&n);
          while (*++s);
          pollEventRate = n;
          break;
        case 'E':
          FIND(Str("no number of graphs"));
          sscanf(s,"%d",&csNGraphs);
          SetCsNGraphs(csNGraphs);
          while (*++s);
          break;
        case 'p':
          SetPlayOnFinish(TRUE);
          break;
        case 'e':
          set_output_format(O, 'e');
          sscanf(s,"%d",&rescale24);
          if (rescale24)
            SetRescaleFloatFileTo24(TRUE);
          SetRescaleFloatFile(TRUE);
          while (*++s);
          break;
        case 'w':
          SetRecordMIDIData(TRUE);
          break;
#endif
          /*******************************************************************/
        case 'C':
          O->usingcscore = 1;     /* use cscore processing  */
          break;
        case 'I':
          O->initonly = 1;           /* I-only implies */
        case 'n':
          O->sfwrite = 0;            /* nosound        */
          break;
        case 'i':
          FIND(Str("no infilename"));
          O->infilename = s;         /* soundin name */
          s += (int) strlen(s);
          if (strcmp(O->infilename,"stdout") == 0)
            csoundDie(csound, Str("input cannot be stdout"));
          if (strcmp(O->infilename,"stdin") == 0) {
            set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mills_macintosh)
            csoundDie(csound, Str("stdin audio not supported"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_SNDFILE, 0);
          O->sfread = 1;
          break;
        case 'o':
          FIND(Str("no outfilename"));
          O->outfilename = s;          /* soundout name */
          s += (int) strlen(s);
          if (strcmp(O->outfilename,"stdin") == 0)
            dieu(Str("-o cannot be stdin"));
          if (strcmp(O->outfilename,"stdout") == 0) {
            set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32) || defined(mills_macintosh)
            csoundDie(csound, Str("stdout audio not supported"));
#endif
          }
          else
            set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 0);
          O->sfwrite = 1;
          break;
        case 'b':
          FIND(Str("no iobufsamps"));
          sscanf(s,"%d%n", &(O->outbufsamps), &n);
          /* defaults in musmon.c */
          O->inbufsamps = O->outbufsamps;
          s += n;
          break;
        case 'B':
          FIND(Str("no hardware bufsamps"));
          sscanf(s,"%d%n", &(O->oMaxLag), &n);
          /* defaults in rtaudio.c */
          s += n;
          break;
        case 'A':
          O->sfheader = 1;
          O->filetyp = TYP_AIFF;     /* AIFF output request*/
          break;
        case 'J':
          O->sfheader = 1;
          O->filetyp = TYP_IRCAM;      /* IRCAM output request */
          break;
        case 'W':
          O->sfheader = 1;
          O->filetyp = TYP_WAV;      /* WAV output request */
          break;
        case 'h':
          O->sfheader = 0;           /* skip sfheader  */
          O->filetyp = TYP_RAW;
          break;
        case 'c':
        case 'a':
        case 'u':
        case '8':
        case 's':
        case '3':
        case 'l':
        case 'f':
          set_output_format(O, c);
          break;
        case 'r':
          FIND(Str("no sample rate"));
          sscanf(s,"%ld", &(O->sr_override));
          while (*++s);
          break;
        case 'j':
          FIND("");
          while (*++s);
          break;
        case 'k':
          FIND(Str("no control rate"));
          sscanf(s,"%ld", &(O->kr_override));
          while (*++s);
          break;
        case 'v':
          O->odebug = 1;    /* verbose otran  */
          break;
        case 'm':
          FIND(Str("no message level"));
          sscanf(s,"%d%n", &(O->msglevel), &n);
          s += n;
          break;
        case 'd':
          O->displays = 0;           /* no func displays */
          break;
        case 'g':
          O->graphsoff = 1;          /* don't use graphics */
          break;
        case 'G':
          O->postscript = 1;         /* Postscript graphics*/
          break;
        case 'x':
          FIND(Str("no xfilename"));
          xfilename = s;            /* extractfile name */
          while (*++s);
          break;
        case 't':
          FIND(Str("no tempo value"));
          {
            int val;
            sscanf(s,"%d%n",&val, &n);/* use this tempo .. */
            s += n;
            if (val < 0) dieu(Str("illegal tempo"));
            else if (val == 0) {
              csound->keep_tmp = 1;
              break;
            }
            else O->cmdTempo = val;
            O->Beatmode = 1;       /* on uninterpreted Beats */
          }
          break;
        case 'L':
          FIND(Str("no Linein score device_name"));
          O->Linename = s;           /* Linein device name */
          s += (int) strlen(s);
          if (!strcmp(O->Linename,"stdin")) {
            set_stdin_assign(csound, STDINASSIGN_LINEIN, 1);
#if defined(mills_macintosh)
            csoundDie(csound, Str("-L: stdin not supported on this platform"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_LINEIN, 0);
          O->Linein = 1;
          break;
        case 'M':
          FIND(Str("no midi device_name"));
          O->Midiname = s;           /* Midi device name */
          s += (int) strlen(s);
          if (!strcmp(O->Midiname,"stdin")) {
            set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#if defined(WIN32) || defined(mills_macintosh)
            csoundDie(csound, Str("-M: stdin not supported on this platform"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 0);
          O->Midiin = 1;
          break;
        case 'F':
          FIND(Str("no midifile name"));
          O->FMidiname = s;          /* Midifile name */
          s += (int) strlen(s);
          if (!strcmp(O->FMidiname,"stdin")) {
            set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#if defined(WIN32) || defined(mills_macintosh)
            csoundDie(csound, Str("-F: stdin not supported on this platform"));
#endif
          }
          else
            set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
          O->FMidiin = 1;            /***************/
          break;
        case 'Q':
          FIND(Str("no MIDI output device"));
          O->Midioutname = s;
          s += (int) strlen(s);
          break;
        case 'R':
          O->rewrt_hdr = 1;
          break;
        case 'H':
          if (isdigit(*s)) {
            sscanf(s, "%d%n", &(O->heartbeat), &n);
            s += n;
          }
          else O->heartbeat = 1;
          break;
        case 'N':
          O->ringbell = 1;        /* notify on completion */
          break;
        case 'T':
          O->termifend = 1;       /* terminate on midifile end */
          break;
        case 'D':
          O->gen01defer = 1;  /* defer GEN01 sample loads
                                until performance time */
          break;
        case 'K':
          peakchunks = 0;     /* Do not write peak information */
          break;
        case 'z':
          {
            int full = 0;
            if (*s != '\0') {
              if (isdigit(*s)) full = *s++ - '0';
            }
            create_opcodlst(csound);
            if (csoundInitModules(csound) != 0)
              longjmp(csound->exitjmp, 1);
            list_opcodes(full);
          }
          longjmp(csound->exitjmp, CSOUND_EXITJMP_SUCCESS);
          break;
        case 'Z':
          csound->dither_output = 1;
          break;
        case '@':
          FIND(Str("No indirection file"));
          {
            FILE *ind = fopen(s, "r");
            if (ind==0) {
              sprintf(errmsg, Str("Cannot open indirection file %s\n"), s);
              dieu(errmsg);
            }
            else {
              readOptions(csound, ind);
              fclose(ind);
            }
            while (*s++); s--;
          }
          break;
        case 'O':
          FIND(Str("no log file"));
          while (*s++); s--;
          break;
        case '-':
#if defined(LINUX)
          if (!(strcmp (s, "sched"))) {           /* ignore --sched */
            while (*(++s));
            break;
          }
          if (!(strncmp(s, "sched=", 6))) {
            while (*(++s));
            break;
          }
#endif
          if (!decode_long(csound, s, argc, argv))
            longjmp(csound->exitjmp, 1);
          while (*(++s));
          break;
        case '+':                                     /* IV - Feb 01 2005 */
          if (parse_option_as_cfgvar(csound, (char*) s - 2) != 0)
            longjmp(csound->exitjmp,1);
          while (*(++s));
          break;
        default:
          sprintf(errmsg,Str("unknown flag -%c"), c);
          dieu(errmsg);
        }
      }
    }
    else {
      char *orcNameMode;
      orcNameMode =
        (char*) csoundQueryGlobalVariable(csound, "::argdecode::orcNameMode");
      if (orcNameMode != NULL && strcmp(orcNameMode, "fail") == 0) {
        csoundMessage(csound, Str("error: orchestra and score name is not "
                                  "allowed in .csoundrc\n"));
        longjmp(csound->exitjmp, 1);
      }
      if (orcNameMode == NULL || strcmp(orcNameMode, "ignore") != 0) {
        if (orchname == NULL)
          orchname = --s;
        else if (scorename == NULL)
          scorename = --s;
        else {
          csound->Message(csound,"argc=%d Additional string \"%s\"\n",argc,--s);
          dieu(Str("too many arguments"));
        }
      }
    }
  } while (--argc);
  return 1;
}

