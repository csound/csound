/*
    argdecode.c:

    Copyright (C) 1998-2013 John ffitch, Victor Lazzarini

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
#include "csoundCore.h"         /*                      ARGDECODE.C     */
#include "soundio.h"
#include "new_opts.h"
#include "csmodule.h"
#include <ctype.h>

static void list_audio_devices(CSOUND *csound, int output);
extern void strset_option(CSOUND *csound, char *s);     /* from str_ops.c */

#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || (((s = *++argv) != NULL) && *s == '-')))  \
      dieu(csound, MSG);

#define STDINASSIGN_SNDFILE     1
#define STDINASSIGN_LINEIN      2
#define STDINASSIGN_MIDIFILE    4
#define STDINASSIGN_MIDIDEV     8

#define STDOUTASSIGN_SNDFILE    1
#define STDOUTASSIGN_MIDIOUT    2

/* IV - Feb 19 2005 */

static inline void set_stdin_assign(CSOUND *csound, int type, int state)
{
    if (state)
      csound->stdin_assign_flg |= type;
    else
      csound->stdin_assign_flg &= (~type);
}

static inline void set_stdout_assign(CSOUND *csound, int type, int state)
{
    if (state)
      csound->stdout_assign_flg |= type;
    else
      csound->stdout_assign_flg &= (~type);
}

/* IV - Feb 19 2005 */
static const char *shortUsageList[] = {
  Str_noop("--help\tprint long usage options"),
  Str_noop("-U unam\trun utility program unam"),
  Str_noop("-C\tuse Cscore processing of scorefile"),
  Str_noop("-j N\tuse N threads in performance"),
  Str_noop("-I\tI-time only orch run"),
  Str_noop("-n\tno sound onto disk"),
  Str_noop("-i fnam\tsound input filename"),
  Str_noop("-o fnam\tsound output filename"),
  Str_noop("-b N\tsample frames (or -kprds) per software sound I/O buffer"),
  Str_noop("-B N\tsamples per hardware sound I/O buffer"),
  Str_noop("-A\tcreate an AIFF format output soundfile"),
  Str_noop("-W\tcreate a WAV format output soundfile"),
  Str_noop("-J\tcreate an IRCAM format output soundfile"),
  Str_noop("-h\tno header on output soundfile"),
  Str_noop("-c\t8-bit signed_char sound samples"),
#ifdef never
  Str_noop("-a\talaw sound samples"),
#endif
  Str_noop("-8\t8-bit unsigned_char sound samples"),
  Str_noop("-u\tulaw sound samples"),
  Str_noop("-s\tshort_int sound samples"),
  Str_noop("-l\tlong_int sound samples"),
  Str_noop("-f\tfloat sound samples"),
  Str_noop("-3\t24bit sound samples"),
  Str_noop("-r N\torchestra srate override"),
  Str_noop("-k N\torchestra krate override"),
  Str_noop("-K\tDo not generate PEAK chunks"),
  Str_noop("-v\tverbose orch translation"),
  Str_noop("-m N\ttty message level. Sum of:"),
  Str_noop("\t\t1=note amps, 2=out-of-range msg, 4=warnings"),
  Str_noop("\t\t0/32/64/96=note amp format (raw,dB,colors)"),
  Str_noop("\t\t128=print benchmark information"),
  Str_noop("-d\tsuppress all displays"),
  Str_noop("-g\tsuppress graphics, use ascii displays"),
  Str_noop("-G\tsuppress graphics, use Postscript displays"),
  Str_noop("-x fnam\textract from score.srt using extract file 'fnam'"),
  Str_noop("-t N\tuse uninterpreted beats of the score, initially at tempo N"),
  Str_noop("-t 0\tuse score.srt for sorted score rather than a temporary"),
  Str_noop("-L dnam\tread Line-oriented realtime score events from device 'dnam'"),
  Str_noop("-M dnam\tread MIDI realtime events from device 'dnam'"),
  Str_noop("-F fnam\tread MIDIfile event stream from file 'fnam'"),
  /*  Str_noop("-P N\tMIDI sustain pedal threshold (0 - 128)"), */
  Str_noop("-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)"),
  Str_noop("-H#\tprint heartbeat style 1, 2 or 3 at each soundfile write"),
  Str_noop("-N\tnotify (ring the bell) when score or miditrack is done"),
  Str_noop("-T\tterminate the performance when miditrack is done"),
  Str_noop("-D\tdefer GEN01 soundfile loads until performance time"),
  Str_noop("-Q dnam\tselect MIDI output device"),
  Str_noop("-z\tList opcodes in this version"),
  Str_noop("-Z\tDither output"),
#if defined(LINUX)
  Str_noop("--sched     set real-time priority and lock memory"),
  Str_noop("            (requires -d and real time audio (-iadc/-odac))"),
  Str_noop("--sched=N   set specified scheduling priority, and lock memory"),
  Str_noop("            (requires -d and real time audio (-iadc/-odac))"),
#endif
  NULL
};

static const char *longUsageList[] = {
  "--format={wav,aiff,au,raw,paf,svx,nist,voc,ircam,w64,mat4,mat5",
  "          pvf,xi,htk,sds,avr,wavex,sd2,flac,caf,wve,ogg,mpc2k,rf64}",
  "--format={alaw,ulaw,schar,uchar,float,double,short,long,24bit,vorbis}",
  Str_noop("\t\t\tSet output file format"),
  Str_noop("--aiff\t\t\tSet AIFF format"),
  Str_noop("--au\t\t\tSet AU format"),
  Str_noop("--wave\t\t\tSet WAV format"),
  Str_noop("--ircam\t\t\tSet IRCAM format"),
  Str_noop("--ogg\t\t\tSet OGG/VORBIS format"),
  Str_noop("--noheader\t\tRaw format"),
  Str_noop("--nopeaks\t\tDo not write peak information"),
  " ",
   Str_noop("--displays\t\tUse graphic displays"),
  Str_noop("--nodisplays\t\tSuppress all displays"),
  Str_noop("--asciidisplay\t\tSuppress graphics, use ascii displays"),
  Str_noop("--postscriptdisplay\tSuppress graphics, use Postscript displays"),
  " ",
  Str_noop("--defer-gen1\t\tDefer GEN01 soundfile loads until performance time"),
  Str_noop("--iobufsamps=N\t\tSample frames (or -kprds) per software "
           "sound I/O buffer"),
  Str_noop("--hardwarebufsamps=N\tSamples per hardware sound I/O buffer"),
  Str_noop("--cscore\t\tUse Cscore processing of scorefile"),
  Str_noop("--orc\t\t\tUse orchfile without scorefile"),
  " ",
  Str_noop("--midifile=FNAME\tRead MIDIfile event stream from file"),
  Str_noop("--midioutfile=FNAME\tWrite MIDI output to file FNAME"),
  Str_noop("--midi-device=FNAME\tRead MIDI realtime events from device"),
  Str_noop("--terminate-on-midi\tTerminate the performance when miditrack is done"),
  " ",
  Str_noop("--heartbeat=N\t\tPrint a heartbeat style 1, 2 or 3 at "
           "each soundfile write"),
  Str_noop("--notify\t\tNotify (ring the bell) when score or miditrack is done"),
  Str_noop("--rewrite\t\tContinually rewrite header while writing "
           "soundfile (WAV/AIFF)"),
  " ",
  Str_noop("--input=FNAME\t\tSound input filename"),
  Str_noop("--output=FNAME\t\tSound output filename"),
  Str_noop("--logfile=FNAME\t\tLog output to file"),
  " ",
  Str_noop("--nosound\t\tNo sound onto disk or device"),
  Str_noop("--tempo=N\t\tUse uninterpreted beats of the score, initially"
           " at tempo N"),
  Str_noop("--i-only\t\tI-time only orch run"),
  Str_noop("--syntax-check-only\tStop after checking orchestra and score syntax"),
  Str_noop("--control-rate=N\tOrchestra krate override"),
  Str_noop("--sample-rate=N\t\tOrchestra srate override"),
  Str_noop("--score-in=FNAME\tRead Line-oriented realtime score events"
           " from device"),
  Str_noop("--messagelevel=N\ttty message level, sum of:"),
  Str_noop("--messageolevel=O\ttty message level in octal, of:"),
  Str_noop("\t\t\t\t1=note amps, 2=out-of-range msg, 4=warnings,"),
  Str_noop("\t\t\t\t0/32/64/96=note amp format (raw,dB,colors),"),
  Str_noop("\t\t\t\t128=print benchmark information"),
  " ",
  Str_noop("--m-amps=[01]\tmessages on note amps"),
  Str_noop("--m-range=[01]\tMessages on range errors"),
  Str_noop("--m-warnings=[01]\tMesage on warnings"),
  Str_noop("--m-raw=[01]\tRaw amp messages"),
  Str_noop("--m-dB=[01]\tAmp messages in dB"),
  Str_noop("--m-colours=[01]\tColour amp messages"),
  Str_noop("--m-benchmarks=[01]\tPrint benchmark information"),
  Str_noop("--csd-line-nums=[01]\tControls how error line numbers are printed:"),
  Str_noop("\t\t\t1=use CSD line #s (default), 0=use ORC/SCO-relative line #s"),
  Str_noop("--extract-score=FNAME\tExtract from score.srt using extract file"),
  Str_noop("--keep-sorted-score"),
  Str_noop("--env:NAME=VALUE\tSet environment variable NAME to VALUE"),
  Str_noop("--env:NAME+=VALUE\tAppend VALUE to environment variable NAME"),
  Str_noop("--strsetN=VALUE\t\tSet strset table at index N to VALUE"),
  Str_noop("--utility=NAME\t\tRun utility program"),
  Str_noop("--verbose\t\tVerbose orch translation"),
  Str_noop("--list-opcodes\t\tList opcodes in this version"),
  Str_noop("--list-opcodesN\t\tList opcodes in style N in this version"),
  Str_noop("--dither\t\tDither output"),
  Str_noop("--dither-triangular\t\tDither output with triangular distribution"),
  Str_noop("--dither-uniform\t\tDither output with rectanular distribution"),
  Str_noop("--sched\t\t\tSet real-time scheduling priority and lock memory"),
  Str_noop("--sched=N\t\tSet priority to N and lock memory"),
  Str_noop("--opcode-lib=NAMES\tDynamic libraries to load"),
  Str_noop("--opcode-omit=NAMES\tDynamic libraries not to load"),
  Str_noop("--omacro:XXX=YYY\tSet orchestra macro XXX to value YYY"),
  Str_noop("--smacro:XXX=YYY\tSet score macro XXX to value YYY"),
  Str_noop("--midi-key=N\t\tRoute MIDI note on message"),
  Str_noop("\t\t\tkey number to pfield N as MIDI value [0-127]"),
  Str_noop("--midi-key-cps=N\tRoute MIDI note on message"),
  Str_noop("\t\t\tkey number to pfield N as cycles per second"),
  Str_noop("--midi-key-oct=N\tRoute MIDI note on message"),
  Str_noop("\t\t\tkey number to pfield N as linear octave"),
  Str_noop("--midi-key-pch=N\tRoute MIDI note on message"),
  Str_noop("\t\t\tkey number to pfield N as oct.pch"),
  Str_noop("--midi-velocity=N\tRoute MIDI note on message"),
  Str_noop("\t\t\tvelocity number to pfield N as MIDI value [0-127]"),
  Str_noop("--midi-velocity-amp=N\tRoute MIDI note on message"),
  Str_noop("\t\t\tvelocity number to pfield N as amplitude"),
  Str_noop("--no-default-paths\tTurn off relative paths from CSD/ORC/SCO"),
  Str_noop("--sample-accurate\t\tUse sample-accurate timing of score events"),
  Str_noop("--realtime\t\trealtime priority mode"),
  Str_noop("--nchnls=N\t\t override number of audio channels"),
  Str_noop("--nchnls_i=N\t\t override number of input audio channels"),
  Str_noop("--0dbfs=N\t\t override 0dbfs (max positive signal amplitude)"),
  Str_noop("--sinesize\t\tlength of internal sine table"),
  Str_noop("--daemon\t\t daemon mode: do not exit if CSD/orchestra is "
           "not given, is empty or does not compile"),
  Str_noop("--port=N\t\t listen to UDP port N for instruments/orchestra "
           "code (implies --daemon)"),
  Str_noop("--vbr-quality=Ft\t set quality of variable bit0rate compression"),
  Str_noop("--devices[=in|out] \t\t list available audio devices and exit"),
  Str_noop("--get-system-sr \t\t print system sr and exit"),
  " ",
  Str_noop("--help\t\t\tLong help"),

  NULL
};

/* IV - Feb 19 2005 */
void print_short_usage(CSOUND *csound)
{
    char    buf[256];
    int     i;
    i = -1;
    while (shortUsageList[++i] != NULL) {
      snprintf(buf, 256, "%s\n", shortUsageList[i]);
      csound->Message(csound, Str(buf));
    }
    csound->Message(csound,
                    Str("flag defaults: csound -s -otest -b%d -B%d -m%d\n"),
                    IOBUFSAMPS, IODACSAMPS, csound->oparms->msglevel);
}

static void longusage(CSOUND *p)
{
    const char **sp;
    p->Message(p, Str("Usage:\tcsound [-flags] orchfile scorefile\n"));
    p->Message(p, Str("Legal flags are:\n"));
    p->Message(p, Str("Long format:\n\n"));
    for (sp = &(longUsageList[0]); *sp != NULL; sp++)
      p->Message(p, "%s\n", Str(*sp));
    /* IV - Feb 19 2005 */
    dump_cfg_variables(p);
    p->Message(p, Str("\nShort form:\n"));
    print_short_usage(p);
    // p->LongJmp(p, 0);
}

void dieu(CSOUND *csound, char *s, ...)
{
    va_list args;

    csound->Message(csound,Str("Usage:\tcsound [-flags] orchfile scorefile\n"));
    csound->Message(csound,Str("Legal flags are:\n"));
    print_short_usage(csound);
    va_start(args, s);
    csound->ErrMsgV(csound, Str("Csound Command ERROR:\t"), s, args);
    va_end(args);
    if(csound->info_message_request == 0){
      csound->info_message_request = 0;
    csound->LongJmp(csound, 1);
    }
}

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

    case 'd':
      O->outformat = AE_DOUBLE;  /* double soundfile */
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

    case 'v':
      O->outformat = AE_VORBIS;  /* Xiph Vorbis encoding */
      break;

    default:
      return; /* do nothing */
    };
}

typedef struct  {
    char    *longformat;
    char    shortformat;
} SAMPLE_FORMAT_ENTRY;

static const SAMPLE_FORMAT_ENTRY sample_format_map[] = {
  { "alaw",   'a' },  { "schar",  'c' },  { "uchar",  '8' },
  { "float",  'f' },  { "double", 'd' },  { "long",   'l' },
  { "short",  's' },  { "ulaw",   'u' },  { "24bit",  '3' },
  { "vorbis", 'v' },  { NULL, '\0' }
};

typedef struct {
    char    *format;
    int     type;
} SOUNDFILE_TYPE_ENTRY;

static const SOUNDFILE_TYPE_ENTRY file_type_map[] = {
    { "wav",    TYP_WAV   },  { "aiff",   TYP_AIFF  },
    { "au",     TYP_AU    },  { "raw",    TYP_RAW   },
    { "paf",    TYP_PAF   },  { "svx",    TYP_SVX   },
    { "nist",   TYP_NIST  },  { "voc",    TYP_VOC   },
    { "ircam",  TYP_IRCAM },  { "w64",    TYP_W64   },
    { "mat4",   TYP_MAT4  },  { "mat5",   TYP_MAT5  },
    { "pvf",    TYP_PVF   },  { "xi",     TYP_XI    },
    { "htk",    TYP_HTK   },  { "sds",    TYP_SDS   },
    { "avr",    TYP_AVR   },  { "wavex",  TYP_WAVEX },
    { "sd2",    TYP_SD2   },
    { "flac",   TYP_FLAC  },  { "caf",    TYP_CAF   },
    { "wve",    TYP_WVE   },  { "ogg",    TYP_OGG   },
    { "mpc2k",  TYP_MPC2K },  { "rf64",   TYP_RF64  },
    { NULL , -1 }
};

extern void sfopenout(CSOUND *csound);
extern void sfcloseout(CSOUND *csound);

static int decode_long(CSOUND *csound, char *s, int argc, char **argv)
{
    OPARMS  *O = csound->oparms;
    /* Add other long options here */
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "decode_long %s\n", s);
    if (!(strncmp(s, "omacro:", 7))) {
      NAMES *nn = (NAMES*) csound->Malloc(csound, sizeof(NAMES));
      nn->mac = s;
      nn->next = csound->omacros;
      csound->omacros = nn;
      return 1;
    }
    else if (!(strncmp(s, "smacro:", 7))) {
      NAMES *nn = (NAMES*) csound->Malloc(csound, sizeof(NAMES));
      nn->mac = s;
      nn->next = csound->smacros;
      csound->smacros = nn;
      return 1;
    }
    else if (!(strncmp(s, "format=", 7))) {
      const SAMPLE_FORMAT_ENTRY   *sfe;
      const SOUNDFILE_TYPE_ENTRY  *ff;
      s += 7;
      do {
        char  *t;
        t = strchr(s, ':');
        if (t != NULL)
          *(t++) = '\0';
        for (ff = &(file_type_map[0]); ff->format != NULL; ff++) {
          if (strcmp(ff->format, s) == 0) {
            O->filetyp = ff->type;
            goto nxtToken;
          }
        }
        for (sfe = &(sample_format_map[0]); sfe->longformat != NULL; sfe++) {
          if (strcmp(s, sfe->longformat) == 0) {
            set_output_format(O, sfe->shortformat);
            goto nxtToken;
          }
        }
        csoundErrorMsg(csound, Str("unknown output format: '%s'"), s);
        return 0;
      nxtToken:
        s = t;
      } while (s != NULL);
      return 1;
    }
    /* -A */
    else if (!(strcmp (s, "aiff"))) {
      O->filetyp = TYP_AIFF;            /* AIFF output request */
      return 1;
    }
    else if (!(strcmp (s, "au"))) {
      O->filetyp = TYP_AU;              /* AU output request */
      return 1;
    }
    else if (!(strncmp (s, "iobufsamps=", 11))) {
      s += 11;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no iobufsamps"));
      /* defaults in musmon.c */
      O->inbufsamps = O->outbufsamps = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "hardwarebufsamps=", 17))) {
      s += 17;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no hardware bufsamps"));
      O->inbufsamps = O->outbufsamps = atoi(s);
      return 1;
    }
    else if (!(strcmp (s, "orc"))) {
      csound->use_only_orchfile = 1;    /* orchfile without scorefile */
      return 1;
    }
    else if (!(strcmp (s, "cscore"))) {
      O->usingcscore = 1;               /* use cscore processing  */
      return 1;
    }
    else if (!(strcmp (s, "nodisplays"))) {
      O->displays = 0;                  /* no func displays */
      return 1;
    }
    else if (!(strcmp (s, "displays"))) {
      O->displays = 1;                  /* func displays */
      O->graphsoff = 0;
      return 1;
    }
    else if (!(strcmp (s, "defer-gen1"))) {
      O->gen01defer = 1;                /* defer GEN01 sample loads */
      return 1;                         /*   until performance time */
    }
    else if (!(strncmp (s, "midifile=", 9))) {
      s += 9;
      if (UNLIKELY(*s == '\0')) dieu(csound, Str("no midifile name"));
      O->FMidiname = s;                 /* Midifile name */
      if (!strcmp(O->FMidiname, "stdin")) {
#if defined(WIN32)
        csoundDie(csound, Str("-F: stdin not supported on this platform"));
#else
        set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
      O->FMidiin = 1;                   /***************/
      return 1;
    }
    else if (!(strncmp (s, "midioutfile=", 12))) {
      s += 12;
      if (UNLIKELY(*s == '\0')) dieu(csound, Str("no midi output file name"));
      O->FMidioutname = s;
      return 1;
    }
    /* -g */
    else if (!(strcmp (s, "asciidisplay"))) {
      O->graphsoff = 1;                 /* don't use graphics but ASCII */
      return 1;
    }
    /* -G */
    else if (!(strcmp (s, "postscriptdisplay"))) {
      O->postscript = 1;                /* don't use graphics but PostScript */
      return 1;
    }
    /* -h */
    else if (!(strcmp (s, "noheader"))) {
      O->filetyp = TYP_RAW;             /* RAW output request */
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
      if (UNLIKELY(python_add_cmdline_definition(s)))
        dieu(csound, Str("invalid python variable definition syntax"));
      return 1;
    }
#endif
    else if (!(strncmp (s, "input=", 6))) {
      s += 6;
      if (UNLIKELY(*s == '\0')) dieu(csound, Str("no infilename"));
      O->infilename = s;                /* soundin name */
      if (UNLIKELY(strcmp(O->infilename, "stdout") == 0))
        csoundDie(csound, Str("input cannot be stdout"));
      if (strcmp(O->infilename, "stdin") == 0) {
        set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32)
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
      csound->initonly = 1;
      O->syntaxCheckOnly = 0;           /* overrides --syntax-check-only */
      O->sfwrite = 0;                   /* and implies nosound */
      return 1;
    }
    /*
      -j Used in localisation
      -J create an IRCAM format output soundfile
     */
    else if (!(strcmp (s, "ircam"))) {
      O->filetyp = TYP_IRCAM;           /* IRCAM output request */
      return 1;
    }
    else if (!(strcmp (s, "ogg"))) {
      O->filetyp = TYP_OGG;             /* OGG output request   */
      O->outformat = AE_VORBIS;         /* Xiph Vorbis encoding */
      return 1;
    }
    /*
      -k N orchestra krate override
     */
    else if (!(strncmp(s, "control-rate=", 13))) {
      s += 13;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no control rate"));
      O->kr_override = (float)atof(s);
      return 1;
    }
    /* -K */
    else if (!(strcmp (s, "nopeaks"))) {
      csound->peakchunks = 0;           /* Do not write peak information */
      return 1;
    }
    /*
      -L dnam read Line-oriented realtime score events from device 'dnam'
     */
    else if (!(strncmp (s, "score-in=", 9))) {
      s += 9;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no Linein score device_name"));
      O->Linename = s;
      if (!strcmp(O->Linename, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_LINEIN, 1);
      }
      else
        set_stdin_assign(csound, STDINASSIGN_LINEIN, 0);
      O->Linein = 1;
      return 1;
    }
    /*
      -m N tty message level.
      Sum of: 1=note amps, 2=out-of-range msg, 4=warnings
     */
    else if (!(strncmp (s, "messagelevel=", 13))) {
      s += 13;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message level"));
      O->msglevel = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "messageolevel=", 14))) {
      s += 14;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message level"));
      sscanf(s, "%o", &(O->msglevel));
      return 1;
    }
    else if (!(strncmp (s, "m-amps=", 7))) {
      int n;
      s += 7;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message amps"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= AMPLMSG;
      else O->msglevel &= ~AMPLMSG;
      return 1;
    }
    else if (!(strncmp (s, "m-range=",8))) {
      int n;
      s += 8;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message range"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= RNGEMSG;
      else O->msglevel &= ~RNGEMSG;
      return 1;
     }
    else if (!(strncmp (s, "m-warnings=",11))) {
      int n;
      s += 11;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message warnings"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= WARNMSG;
      else O->msglevel &= ~WARNMSG;
      return 1;
    }
    else if (!(strncmp (s, "m-raw=",6))) {
      int n;
      s += 6;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message raw"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= 32;
      else O->msglevel &= ~32;
      return 1;
    }
    else if (!(strncmp (s, "m-dB=",5))) {
      int n;
      s += 5;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no message dB"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= RAWMSG;
      else O->msglevel &= ~RAWMSG;
      return 1;
    }
    else if (!(strncmp (s, "m-colours=",10)) || !(strncmp (s, "m-colors=",9))) {
      int n;
      s += 10;
      if (*s=='\0') dieu(csound, Str("no message colours"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= 96;
      else O->msglevel &= ~96;
      return 1;
    }
    else if (!(strncmp (s, "m-benchmarks=",13))) {
      int n;
      s += 13;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no benchmark level"));
      sscanf(s, "%d", &n);
      if (n) O->msglevel |= TIMEMSG;
      else O->msglevel &= ~TIMEMSG;
      return 1;
    }
    else if (!(strncmp (s, "csd-line-nums=",14))) {
      s += 14;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no value for --csd-line-nums"));
      O->useCsdLineCounts = (atoi(s) != 0);
      return 1;
    }
    /*
      -M dnam read MIDI realtime events from device 'dnam'
     */
    else if (!(strncmp (s, "midi-device=", 12))) {
      s += 12;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no midi device_name"));
      O->Midiname = s;
      if (!strcmp(O->Midiname, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#if defined(WIN32)
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
      O->sfwrite = 0;                   /* nosound        */
      return 1;
    }
    /* -N */
    else if (!(strcmp (s, "notify"))) {
      O->ringbell = 1;                  /* notify on completion */
      return 1;
    }
    else if (!(strncmp (s, "output=", 7))) {
      s += 7;
      if (UNLIKELY(*s == '\0')) dieu(csound, Str("no outfilename"));
      O->outfilename = s;               /* soundout name */
      if (UNLIKELY(strcmp(O->outfilename, "stdin")) == 0)
        dieu(csound, Str("-o cannot be stdin"));
      if (strcmp(O->outfilename, "stdout") == 0) {
        set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32)
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
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no log file"));
      return 1;
    }
    /* -r N */
    else if (!(strncmp (s, "sample-rate=", 12))) {
      s += 12;
      O->sr_override = (float)atof(s);
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
    else if (UNLIKELY(!(strncmp (s, "tempo=", 6)))) {
      s += 6;
      O->cmdTempo = atoi(s);
      O->Beatmode = 1;                  /* on uninterpreted Beats */
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
    else if (!(strcmp (s, "no-expression-opt"))) {
      O->expr_opt = 0;
      return 1;
    }
    else if (!(strncmp (s, "env:", 4))) {
      if (csoundParseEnv(csound, s + 4) == CSOUND_SUCCESS)
        return 1;
      else
        return 0;
    }
    else if (!(strncmp (s, "strset", 6))) {
      strset_option(csound, s + 6);
      return 1;
    }
    /* -T terminate the performance when miditrack is done */
    else if (!(strcmp (s, "terminate-on-midi"))) {
      O->termifend = 1;                 /* terminate on midifile end */
      return 1;
    }
    else if (!(strncmp (s, "utility=", 8))) {
      int retval;
      s += 8;
      if (*s=='\0') dieu(csound, Str("no utility name"));

      retval = csoundRunUtility(csound, s, argc, argv);
     if(retval) {
                  csound->info_message_request = 1;
                  csound->orchname = NULL;
                  return 0;
              }
       else csound->LongJmp(csound, retval);
     return 1;
    }
    /* -v */
    else if (!(strcmp (s, "verbose"))) {
      O->odebug = 1;                    /* verbose otran  */
      return 1;
    }
    /* -x fnam extract from score.srt using extract file 'fnam' */
    else if (!(strncmp(s, "extract-score=", 14))) {
      s += 14;
      if (UNLIKELY(*s=='\0')) dieu(csound, Str("no xfilename"));
      csound->xfilename = s;
      return 1;
    }
    else if (!(strcmp(s, "wave"))) {
      O->filetyp = TYP_WAV;             /* WAV output request */
      return 1;
    }
    else if (!(strncmp (s, "list-opcodes", 12))) {
      int full = 0;
      s += 12;

      if (*s != '\0') {
        if (isdigit(*s)) full = *s++ - '0';
      }
      /* VL: moved to csoundReset() in csound.c
              csoundLoadExternals(csound);
              if (csoundInitModules(csound) != 0)
              csound->LongJmp(csound, 1); */
      list_opcodes(csound, full);
      return 1;
      //csound->LongJmp(csound, 0);
    }
    /* -Z */
    else if (!(strcmp (s, "dither"))) {
      csound->dither_output = 1;
      return 1;
    }
    else if (!(strcmp (s, "dither-uniform"))) {
      csound->dither_output = 2;
      return 1;
    }
    else if (!(strcmp (s, "dither-triangular"))) {
      csound->dither_output = 1;
      return 1;
    }
    else if (!(strncmp (s, "midi-key=", 9))) {
      s += 9;
      O->midiKey = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-key-cps=", 13))) {
      s += 13 ;
      O->midiKeyCps = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-key-oct=", 13))) {
      s += 13 ;
      O->midiKeyOct = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-key-pch=", 13))) {
      s += 13 ;
      O->midiKeyPch = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-velocity=", 14))) {
      s += 14;
      O->midiVelocity = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "midi-velocity-amp=", 18))) {
      s += 18;
      O->midiVelocityAmp = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "opcode-lib=", 11))) {
      int   nbytes;
      s += 11;
      nbytes = (int) strlen(s) + 1;
      if (csound->dl_opcodes_oplibs == NULL) {
        /* start new library list */
        csound->dl_opcodes_oplibs = (char*) csound->Malloc(csound, (size_t) nbytes);
        strcpy(csound->dl_opcodes_oplibs, s);
      }
      else {
        /* append to existing list */
        nbytes += ((int) strlen(csound->dl_opcodes_oplibs) + 1);
        csound->dl_opcodes_oplibs = (char*) csound->ReAlloc(csound,
                                                     csound->dl_opcodes_oplibs,
                                                     (size_t) nbytes);
        strcat(csound->dl_opcodes_oplibs, ",");
        strcat(csound->dl_opcodes_oplibs, s);
      }
      return 1;
    }
    else if (!(strcmp(s, "default-paths"))) {
      O->noDefaultPaths = 0;
      return 1;
    }
    else if (!(strcmp(s, "no-default-paths"))) {
      O->noDefaultPaths = 1;
      return 1;
    }
    else if (!(strncmp (s, "num-threads=", 12))) {
      s += 12 ;
      O->numThreads = atoi(s);
      return 1;
    }
    else if (!(strcmp (s, "syntax-check-only"))) {
      O->syntaxCheckOnly = 1;
      return 1;
    }
    else if (!(strcmp(s, "help"))) {
      longusage(csound);
      csound->info_message_request = 1;
      return 1;
      //csound->LongJmp(csound, 0);
    }
    else if (!(strcmp(s, "sample-accurate"))) {
      O->sampleAccurate = 1;
      return 1;
    }
    else if (!(strcmp(s, "realtime"))) {
      csound->Message(csound, "realtime mode enabled\n");
      O->realtime = 1;
      return 1;
    }
    else if (!(strncmp(s, "nchnls=", 7))) {
      s += 7;
      O->nchnls_override = atoi(s);
      return 1;
    }
    else if (!(strncmp(s, "0dbfs=", 6))) {
      s += 6;
      O->e0dbfs_override = atoi(s);
      return 1;
    }
    else if (!(strncmp(s, "nchnls_i=", 9))) {
      s += 9;
      O->nchnls_i_override = atoi(s);
      return 1;
    }
    else if (!(strncmp(s, "sinesize=", 9))) {
      {
        int i = 1, n;
        s += 9;
        n = atoi(s);
        while (i<=n && i< MAXLEN) i <<= 1;
        csound->sinelength = i;
        return 1;
      }
    }
    else if (!(strcmp(s, "new-parser")) ||
             !(strcmp(s, "old-parser"))) {
        return 1;  /* ignore flag, this is here for backwards compatibility */
    }
    else if (!(strcmp(s, "daemon"))) {
        O->daemon = 1;
        return 1;
    }
    else if (!(strncmp(s, "port=",5))) {
        s += 5;
        O->daemon = atoi(s);
        return 1;
    }
    else if (!(strncmp(s, "vbr-quality=",12))) {
        s += 12;
        O->quality = atof(s);
        return 1;
      }
    else if (!(strncmp(s, "devices",7))) {
      csoundLoadExternals(csound);
      if (csoundInitModules(csound) != 0)
              csound->LongJmp(csound, 1);
      if(*(s+7) == '='){
        if(!strncmp(s+8,"in", 2)) {
          list_audio_devices(csound, 0);
        }
        else if(!strncmp(s+8,"out", 2))
          list_audio_devices(csound,1);
      }
      else {
        list_audio_devices(csound,0);
        list_audio_devices(csound,1);
      }
      csound->info_message_request = 1;
      return 1;
      }
    else if (!(strncmp(s, "get-system-sr",13))){
      if(O->outfilename && 
        !(strncmp(O->outfilename, "dac",3))) {
      /* these are default values to get the
         backend to open successfully */
      set_output_format(O, 'f');
      O->inbufsamps = O->outbufsamps = 256;
      O->oMaxLag = 1024;
      csoundLoadExternals(csound);
      if (csoundInitModules(csound) != 0)
              csound->LongJmp(csound, 1);
      sfopenout(csound);
      csound->Message(csound, "system sr: %f\n", csound->system_sr(csound,0));
      sfcloseout(csound);
      }
      csound->info_message_request = 1;
      return 1;
    }

    csoundErrorMsg(csound, Str("unknown long option: '--%s'"), s);
    return 0;
}

PUBLIC int argdecode(CSOUND *csound, int argc, char **argv_)
{
    OPARMS  *O = csound->oparms;
    char    *s, **argv;
    int     n;
    char    c;

    /* make a copy of the option list */
    char  *p1, *p2;
    int   nbytes, i;
    /* calculate the number of bytes to allocate */
    /* N.B. the argc value passed to argdecode is decremented by one */
    nbytes = (argc + 1) * (int) sizeof(char*);
    for (i = 0; i <= argc; i++)
      nbytes += ((int) strlen(argv_[i]) + 1);
    p1 = (char*) csound->Malloc(csound, nbytes);   /* will be freed by memRESET() */
    p2 = (char*) p1 + ((int) sizeof(char*) * (argc + 1));
    argv = (char**) p1;
    for (i = 0; i <= argc; i++) {
      argv[i] = p2;
      strcpy(p2, argv_[i]);
      p2 = (char*) p2 + ((int) strlen(argv_[i]) + 1);
    }
    csound->keep_tmp = 0;

    do {
      s = *++argv;
      if (*s++ == '-') {                  /* read all flags:  */
        while ((c = *s++) != '\0') {
          switch (c) {
          case 'U':
            FIND(Str("no utility name"));
            {
              int retval = csoundRunUtility(csound, s, argc, argv);
              if(retval) {
                  csound->info_message_request = 1;
                  csound->orchname = NULL;
                  goto end;
              }
              else csound->LongJmp(csound, retval);
            }
            break;
          case 'C':
            O->usingcscore = 1;           /* use cscore processing  */
            break;
          case 'I':
            csound->initonly = 1;         /* I-only overrides */
            O->syntaxCheckOnly = 0;       /* --syntax-check-only and implies */
          case 'n':
            O->sfwrite = 0;               /* nosound        */
            break;
          case 'i':
            FIND(Str("no infilename"));
            O->infilename = s;            /* soundin name */
            s += (int) strlen(s);
            if (UNLIKELY(strcmp(O->infilename, "stdout")) == 0)
              csoundDie(csound, Str("input cannot be stdout"));
            if (strcmp(O->infilename, "stdin") == 0) {
              set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32)
              csoundDie(csound, Str("stdin audio not supported"));
#endif
            }
            else
              set_stdin_assign(csound, STDINASSIGN_SNDFILE, 0);
            O->sfread = 1;
            break;
          case 'o':
            FIND(Str("no outfilename"));
            O->outfilename = s;           /* soundout name */
            s += (int) strlen(s);
            if (UNLIKELY(strcmp(O->outfilename, "stdin") == 0))
              dieu(csound, Str("-o cannot be stdin"));
            if (strcmp(O->outfilename, "stdout") == 0) {
              set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32)
              csoundDie(csound, Str("stdout audio not supported"));
#endif
            }
            else
              set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 0);
            O->sfwrite = 1;
            break;
          case 'b':
            FIND(Str("no iobufsamps"));
            sscanf(s, "%d%n", &(O->outbufsamps), &n);
            /* defaults in musmon.c */
            O->inbufsamps = O->outbufsamps;
            s += n;
            break;
          case 'B':
            FIND(Str("no hardware bufsamps"));
            sscanf(s, "%d%n", &(O->oMaxLag), &n);
            /* defaults in rtaudio.c */
            s += n;
            break;
          case 'A':
            O->filetyp = TYP_AIFF;        /* AIFF output request*/
            break;
          case 'J':
            O->filetyp = TYP_IRCAM;       /* IRCAM output request */
            break;
          case 'W':
            O->filetyp = TYP_WAV;         /* WAV output request */
            break;
          case 'h':
            O->filetyp = TYP_RAW;         /* RAW output request */
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
            O->sr_override = (float)atof(s);
            while (*++s);
            break;
          case 'k':
            FIND(Str("no control rate"));
            O->kr_override = (float)atof(s);
            while (*++s);
            break;
          case 'v':
            O->odebug = 1;                /* verbose otran  */
            break;
          case 'm':
            FIND(Str("no message level"));
            sscanf(s, "%d%n", &(O->msglevel), &n);
            s += n;
            break;
          case 'd':
            O->displays = 0;              /* no func displays */
            break;
          case 'g':
            O->graphsoff = 1;             /* don't use graphics */
            break;
          case 'G':
            O->postscript = 1;            /* Postscript graphics*/
            break;
          case 'x':
            FIND(Str("no xfilename"));
            csound->xfilename = s;        /* extractfile name */
            while (*++s);
            break;
          case 't':
            FIND(Str("no tempo value"));
            {
              int val;
              sscanf(s, "%d%n", &val, &n); /* use this tempo .. */
              s += n;
              if (UNLIKELY(val < 0)) dieu(csound, Str("illegal tempo"));
              else if (val == 0) {
                csound->keep_tmp = 1;
                break;
              }
              O->cmdTempo = val;
              O->Beatmode = 1;            /* on uninterpreted Beats */
            }
            break;
          case 'L':
            FIND(Str("no Linein score device_name"));
            O->Linename = s;              /* Linein device name */
            s += (int) strlen(s);
            if (!strcmp(O->Linename, "stdin")) {
              set_stdin_assign(csound, STDINASSIGN_LINEIN, 1);
            }
            else
              set_stdin_assign(csound, STDINASSIGN_LINEIN, 0);
            O->Linein = 1;
            break;
          case 'M':
            FIND(Str("no midi device_name"));
            O->Midiname = s;              /* Midi device name */
            s += (int) strlen(s);
            if (strcmp(O->Midiname, "stdin")==0) {
#if defined(WIN32)
              csoundDie(csound, Str("-M: stdin not supported on this platform"));
#else
              set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#endif
            }
            else
              set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 0);
            O->Midiin = 1;
            break;
          case 'F':
            FIND(Str("no midifile name"));
            O->FMidiname = s;             /* Midifile name */
            s += (int) strlen(s);
            if (strcmp(O->FMidiname, "stdin")==0) {
#if defined(WIN32)
              csoundDie(csound, Str("-F: stdin not supported on this platform"));
#else
              set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#endif
            }
            else
              set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
            O->FMidiin = 1;               /*****************/
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
            O->ringbell = 1;              /* notify on completion */
            break;
          case 'T':
            O->termifend = 1;             /* terminate on midifile end */
            break;
          case 'D':
            O->gen01defer = 1;            /* defer GEN01 sample loads
                                             until performance time */
            break;
          case 'K':
            csound->peakchunks = 0;       /* Do not write peak information */
            break;
          case 'z':
            {
              int full = 0;
              if (*s != '\0') {
                if (isdigit(*s)) full = *s++ - '0';
              }
              /* VL: moved to csoundReset() in csound.c
              csoundLoadExternals(csound);
              if (csoundInitModules(csound) != 0)
              csound->LongJmp(csound, 1); */
              list_opcodes(csound, full);
            }
            csound->info_message_request = 1;
            // csound->LongJmp(csound, 0);
            break;
          case 'Z':
            {
              int full = 1;
              if (*s != '\0') {
                if (isdigit(*s)) full = *s++ - '0';
              }
              csound->dither_output = full;
            }
            break;
          case '@':
            FIND(Str("No indirection file"));
            {
              FILE *ind;
              void *fd;
              fd = csound->FileOpen2(csound, &ind, CSFILE_STD,
                                     s, "rb", NULL, CSFTYPE_OPTIONS, 0);
              if (UNLIKELY(fd == NULL)) {
                dieu(csound, Str("Cannot open indirection file %s\n"), s);
              }
              else {
                readOptions(csound, ind, 0);
                csound->FileClose(csound, fd);
              }
              while (*s++)
               ; s--; /* semicolon on separate line to silence warning */
            }
            break;
          case 'O':
            FIND(Str("no log file"));
            while (*s++)
              ; s--; /* semicolon on separate line to silence warning */
            break;
          case '-':
#if defined(LINUX)
            if (!(strcmp (s, "sched"))) {             /* ignore --sched */
              while (*(++s));
              break;
            }
            if (!(strncmp(s, "sched=", 6))) {
              while (*(++s));
              break;
            }
#endif
            if (!decode_long(csound, s, argc, argv))
              csound->LongJmp(csound, 1);
            while (*(++s));
            break;
          case 'j':
            FIND(Str("no number of threads"));
            sscanf(s, "%d%n", &(O->numThreads), &n);
            s += n;
            break;
          case '+':                                   /* IV - Feb 01 2005 */
            if (parse_option_as_cfgvar(csound, (char*) s - 2) != 0)
              csound->LongJmp(csound, 1);
            while (*(++s));
            break;
          default:
            if(csound->info_message_request == 0)
            dieu(csound, Str("unknown flag -%c"), c);
          }
        }
      }
      else {
        /* 0: normal, 1: ignore, 2: fail */
        if (csound->orcname_mode == 2) {
          csound->Die(csound, Str("error: orchestra and score name not "
                                  "allowed in .csound6rc"));
        }
        else if (csound->orcname_mode == 0) {
          if (csound->orchname == NULL)
            csound->orchname = --s;
          else if (LIKELY(csound->scorename == NULL))
            csound->scorename = --s;
          else {
            csound->Message(csound,"argc=%d Additional string \"%s\"\n",argc,--s);
            dieu(csound, Str("too many arguments"));
          }
        }
      }
    } while (--argc);
 end:
    return 1;
}

PUBLIC int csoundSetOption(CSOUND *csound, char *option){
  /* if already compiled and running, return */
  csound->info_message_request = 1;
  if(csound->engineStatus & CS_STATE_COMP) return 1;
  char *args[2] = {"csound", option};
  return (argdecode(csound, 1, args) ? 0 : 1);
}

PUBLIC void csoundSetParams(CSOUND *csound, CSOUND_PARAMS *p){

  /* if already compiled and running, return */
  if(csound->engineStatus & CS_STATE_COMP) return;

  OPARMS *oparms = csound->oparms;
  /* simple ON/OFF switches */
  oparms->odebug = p->debug_mode;
  oparms->displays = p->displays;
  oparms->graphsoff = p->ascii_graphs;
  oparms->postscript = p->postscript_graphs;
  oparms->usingcscore = p->use_cscore;
  oparms->ringbell = p->ring_bell;
  oparms->gen01defer = p->defer_gen01_load;
  oparms->termifend =  p->terminate_on_midi;
  oparms->noDefaultPaths = p->no_default_paths;
  oparms->syntaxCheckOnly = p->syntax_check_only;
  oparms->sampleAccurate = p->sample_accurate;
  oparms->realtime = p->realtime_mode;
  oparms->useCsdLineCounts = p->csd_line_counts;
  oparms->heartbeat = p->heartbeat;
  oparms->ringbell = p->ring_bell;
  oparms->daemon = p->daemon;

  /* message level */
  if(p->message_level > 0)
   oparms->msglevel = p->message_level;

  /* tempo / beatmode */
  if(p->tempo > 0) {
    oparms->cmdTempo = p->tempo;
    oparms->Beatmode = 1;
  }
  /* buffer frames */
  if(p->buffer_frames > 0)
    oparms->inbufsamps = oparms->outbufsamps = p->buffer_frames;

  /* hardware buffer frames */
  if(p->hardware_buffer_frames > 0)
    oparms->oMaxLag = p->hardware_buffer_frames;

  /* multicore threads */
  if(p->number_of_threads > 1)
    oparms->numThreads = p->number_of_threads;

  /* MIDI interop */
  if(p->midi_key > 0) oparms->midiKey = p->midi_key;
  else if(p->midi_key_cps > 0) oparms->midiKeyCps = p->midi_key_cps;
  else if(p->midi_key_pch > 0) oparms->midiKeyPch = p->midi_key_pch;
  else if(p->midi_key_oct > 0) oparms->midiKeyOct = p->midi_key_oct;

  if(p->midi_velocity > 0) oparms->midiVelocity = p->midi_velocity;
  else if(p->midi_velocity_amp > 0) oparms->midiVelocityAmp = p->midi_velocity_amp;

  /* CSD line counts */
  if(p->csd_line_counts > 0) oparms->useCsdLineCounts = p->csd_line_counts;

  /* kr override */
  if(p->control_rate_override > 0) oparms->kr_override = p->control_rate_override;

  /* sr override */
  if(p->sample_rate_override > 0) oparms->sr_override = p->sample_rate_override;

  oparms->nchnls_override = p->nchnls_override;
  oparms->nchnls_i_override = p->nchnls_i_override;
  oparms->e0dbfs_override = p->e0dbfs_override;
}

PUBLIC void csoundGetParams(CSOUND *csound, CSOUND_PARAMS *p){

  OPARMS *oparms = csound->oparms;

  p->debug_mode = oparms->odebug;
  p->displays = oparms->displays;
  p->ascii_graphs = oparms->graphsoff;
  p->postscript_graphs = oparms->postscript;
  p->use_cscore = oparms->usingcscore;
  p->ring_bell = oparms->ringbell;
  p->defer_gen01_load = oparms->gen01defer;
  p->terminate_on_midi = oparms->termifend;
  p->no_default_paths = oparms->noDefaultPaths;
  p->syntax_check_only = oparms->syntaxCheckOnly;
  p->sample_accurate = oparms->sampleAccurate;
  p->realtime_mode = oparms->realtime;
  p->message_level = oparms->msglevel;
  p->tempo = oparms->cmdTempo;
  p->buffer_frames = oparms->outbufsamps;
  p->hardware_buffer_frames = oparms->oMaxLag;
  p->number_of_threads = oparms->numThreads;
  p->midi_key = oparms->midiKey;
  p->midi_key_cps = oparms->midiKeyCps;
  p->midi_key_pch = oparms->midiKeyPch;
  p->midi_key_oct = oparms->midiKeyOct;
  p->midi_velocity = oparms->midiVelocity;
  p->midi_velocity_amp = oparms->midiVelocityAmp;
  p->csd_line_counts = oparms->useCsdLineCounts;
  p->control_rate_override = oparms->kr_override;
  p->sample_rate_override = oparms->sr_override;
  p->nchnls_override = oparms->nchnls_override;
  p->nchnls_i_override = oparms->nchnls_i_override;
  p->e0dbfs_override = oparms->e0dbfs_override;
  p->heartbeat = oparms->heartbeat;
  p->ring_bell = oparms->ringbell;
  p->daemon = oparms->daemon;
}


PUBLIC void csoundSetOutput(CSOUND *csound, char *name, char *type, char *format)
{

    OPARMS *oparms = csound->oparms;
    char *typename;

    /* if already compiled and running, return */
    if (csound->engineStatus & CS_STATE_COMP) return;

    oparms->outfilename =
      csound->Malloc(csound, strlen(name) + 1); /* will be freed by memRESET */
    strcpy(oparms->outfilename, name); /* unsafe -- REVIEW */
    if (strcmp(oparms->outfilename, "stdout") == 0) {
      set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 1);
#if defined(WIN32)
      csound->Warning(csound, Str("stdout not supported on this platform"));
#endif
    }
    else set_stdout_assign(csound, STDOUTASSIGN_SNDFILE, 0);

    oparms->sfwrite = 1;
    if (type != NULL) {
      int i=0;
      while ((typename = file_type_map[i].format) != NULL) {
        if(!strcmp(type,typename)) break;
        i++;
      }
      if (typename != NULL) {
        oparms->filetyp = file_type_map[i].type;
      }
    }
    if (format != NULL) {
      int i=0;
      while ((typename = sample_format_map[i].longformat) != NULL) {
        if (!strcmp(type,typename)) break;
        i++;
      }
      if (format != NULL) {
        set_output_format(oparms, sample_format_map[i].shortformat);
      }
    }
}

PUBLIC void csoundSetInput(CSOUND *csound, char *name) {
  OPARMS *oparms = csound->oparms;

  /* if already compiled and running, return */
  if(csound->engineStatus & CS_STATE_COMP) return;

  oparms->infilename =
    csound->Malloc(csound, strlen(name)); /* will be freed by memRESET */
  strcpy(oparms->infilename, name);
  if (strcmp(oparms->infilename, "stdin") == 0) {
        set_stdin_assign(csound, STDINASSIGN_SNDFILE, 1);
#if defined(WIN32)
        csound->Warning(csound, Str("stdin not supported on this platform"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_SNDFILE, 0);
  oparms->sfread = 1;
}

PUBLIC void csoundSetMIDIInput(CSOUND *csound, char *name) {
   OPARMS *oparms = csound->oparms;

   /* if already compiled and running, return */
  if(csound->engineStatus & CS_STATE_COMP) return;

   oparms->Midiname =
     csound->Malloc(csound, strlen(name)); /* will be freed by memRESET */
   strcpy(oparms->Midiname, name);
   if (!strcmp(oparms->Midiname, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 1);
#if defined(WIN32)
        csound->Warning(csound, Str("stdin not supported on this platform"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_MIDIDEV, 0);
      oparms->Midiin = 1;
}

PUBLIC void csoundSetMIDIFileInput(CSOUND *csound, char *name) {
   OPARMS *oparms = csound->oparms;

   /* if already compiled and running, return */
  if(csound->engineStatus & CS_STATE_COMP) return;

   oparms->FMidiname =
     csound->Malloc(csound, strlen(name)); /* will be freed by memRESET */
   strcpy(oparms->FMidiname, name);
   if (!strcmp(oparms->FMidiname, "stdin")) {
        set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 1);
#if defined(WIN32)
        csound->Warning(csound, Str("stdin not supported on this platform"));
#endif
      }
      else
        set_stdin_assign(csound, STDINASSIGN_MIDIFILE, 0);
      oparms->FMidiin = 1;
}

PUBLIC void csoundSetMIDIFileOutput(CSOUND *csound, char *name) {
   OPARMS *oparms = csound->oparms;

   /* if already compiled and running, return */
  if(csound->engineStatus & CS_STATE_COMP) return;

   oparms->FMidioutname =
     csound->Malloc(csound, strlen(name)); /* will be freed by memRESET */
   strcpy(oparms->FMidioutname, name);
}

PUBLIC void csoundSetMIDIOutput(CSOUND *csound, char *name) {
   OPARMS *oparms = csound->oparms;

    /* if already compiled and running, return */
  if(csound->engineStatus & CS_STATE_COMP) return;

   oparms->Midioutname =
     csound->Malloc(csound, strlen(name)); /* will be freed by memRESET */
   strcpy(oparms->Midioutname, name);
}

static void list_audio_devices(CSOUND *csound, int output){

       int i,n = csoundGetAudioDevList(csound,NULL, output);
         CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *)
             malloc(n*sizeof(CS_AUDIODEVICE));
         if(output)
          csound->Message(csound, "%d audio output devices \n", n);
         else
           csound->Message(csound, "%d audio input devices \n", n);
         csoundGetAudioDevList(csound,devs,output);
         for(i=0; i < n; i++)
             csound->Message(csound, " %d: %s (%s)\n",
                   i, devs[i].device_id, devs[i].device_name);
         free(devs);
}
