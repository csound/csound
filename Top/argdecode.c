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
#include "soundio.h"
#include <ctype.h>

#ifdef mills_macintosh
#include <SIOUX.h>
#include "perf.h"
#include"MacTransport.h"

#define PATH_LEN        128

extern struct Transport transport;
extern Boolean displayText;

extern char sfdir_path[];
extern char sadir_path[];
extern char ssdir_path[];
extern char saved_scorename[];
extern unsigned char mytitle[];
extern Boolean util_perf;
extern unsigned short pollEventRate;

static char *foo;
static char listing_file[PATH_LEN];
static int  vbuf;
static int csNGraphs;
static int rescale24 = 0;
static MYFLT temp;
#endif

#ifdef LINUX
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
extern void openMIDIout(void);
#endif

static char outformch;
extern  OPARMS  O;
extern  ENVIRON cenviron;
static int     stdinassgn = 0;
extern void create_opcodlst(void);
extern void readOptions(FILE*);

#define FIND(MSG)   if (*s == '\0')  \
                      if (!(--argc) || (((s = *++argv) != NULL) && *s == '-')) \
                         dieu(MSG);

/* alphabetically, so i dont have to hunt for a letter next time...
**********************************************************************
-a alaw sound samples *************NOT NEEDED***************
-A create an AIFF format output soundfile
-b N sample frames (or -kprds) per software sound I/O buffer
-B N samples per hardware sound I/O buffer
-c 8-bit signed_char sound samples
-C use Cscore processing of scorefile
-d suppress all displays
-D defer GEN01 soundfile loads until performance time
-e Rescale floats as shorts to max amplitude
-E (was -G) N  Number of tables in graphics window
-f float sound samples
-F fnam read MIDIfile event stream from file 'fnam'
-g suppress graphics, use ascii displays
-G suppress graphics, use Postscript displays
-h no header on output soundfile
-H N print a heartbeat style 1, 2 or 3 at each soundfile write
-i fnam sound input filename
-I I-time only orch run
-j Used in localisation
-J create an IRCAM format output soundfile
-k N orchestra krate override
-K No Peak Chunks
-l long_int sound samples
-L dnam read Line-oriented realtime score events from device 'dnam'
-m N tty message level. Sum of: 1=note amps, 2=out-of-range msg, 4=warnings
-M dnam read MIDI realtime events from device 'dnam'
-n no sound onto disk
-N notify (ring the bell) when score or miditrack is done
-o fnam sound output filename
-O fnam log output to file
-p Play after rendering
-P N Poll Events Every N Buffer Writes (Mac)
-q fnam  Sound Sample-In Directory
-Q fnam  Analysis Directory (Macintosh)
         MIDI output device (Linux, BeOS)
-r N orchestra srate override
-R continually rewrite header while writing soundfile (WAV/AIFF)
-s short_int sound samples
-S *** was *** score is in Scot format
-t N use uninterpreted beats of the score, initially at tempo N
-t0 Use score.srt rather than a temporary
-T terminate the performance when miditrack is done
-u ulaw sound samples ******* NOT NEEDED *****************
-U unam run utility program unam
-v verbose orch translation
-V N  Number of chars in screen buffer for output window
-w   Record and Save MIDI input to a file
-W create a WAV format output soundfile
-x fnam extract from score.srt using extract file 'fnam'
-X fnam  Sound File Directory
-y N  Enables Profile Display at rate N in seconds,
   or for negative N, at -N kperiods
-Y N  Enables Progress Display at rate N seconds,
   or for negative N, at -N kperiods
-z List opcodes in this version
-Z Dither output
--sched real-time scheduling and memory lock (LINUX only)
-3 24bit samples
-8 8-bit unsigned_char sound samples  J. Mohr 1995 Oct 17
*/

void usage(void)
{
err_printf(Str(X_519,"Usage:\tcsound [-flags] orchfile scorefile\n"));
err_printf(Str(X_325,"Legal flags are:\n"));
err_printf(Str(X_128,"-U unam\trun utility program unam\n"));
err_printf(Str(X_102,"-C\tuse Cscore processing of scorefile\n"));
err_printf(Str(X_109,"-I\tI-time only orch run\n"));
err_printf(Str(X_155,"-n\tno sound onto disk\n"));
err_printf(Str(X_150,"-i fnam\tsound input filename\n"));
err_printf(Str(X_157,"-o fnam\tsound output filename\n"));
err_printf(Str(X_138,"-b N\tsample frames (or -kprds) per software sound I/O buffer\n"));
err_printf(Str(X_100,"-B N\tsamples per hardware sound I/O buffer\n"));
err_printf(Str(X_96,"-A\tcreate an AIFF format output soundfile\n"));
err_printf(Str(X_132,"-W\tcreate a WAV format output soundfile\n"));
err_printf(Str(X_111,"-J\tcreate an IRCAM format output soundfile\n"));
err_printf(Str(X_149,"-h\tno header on output soundfile\n"));
err_printf(Str(X_141,"-c\t8-bit signed_char sound samples\n"));
#ifdef never
err_printf(Str(X_136,"-a\talaw sound samples\n"));
#endif
err_printf(Str(X_94,"-8\t8-bit unsigned_char sound samples\n")); /* J. Mohr 1995 Oct 17 */
#ifdef ULAW
err_printf(Str(X_166,"-u\tulaw sound samples\n"));
#endif
err_printf(Str(X_164,"-s\tshort_int sound samples\n"));
err_printf(Str(X_153,"-l\tlong_int sound samples\n"));
err_printf(Str(X_145,"-f\tfloat sound samples\n"));
err_printf(Str(X_1138,"-3\t24bit sound samples\n"));
err_printf(Str(X_161,"-r N\torchestra srate override\n"));
err_printf(Str(X_152,"-k N\torchestra krate override\n"));
err_printf(Str(X_1552,"-K\tDo not generate PEAK chunks\n"));
err_printf(Str(X_168,"-v\tverbose orch translation\n"));
err_printf(Str(X_154,"-m N\ttty message level. Sum of: 1=note amps, 2=out-of-range msg, 4=warnings\n"));
err_printf(Str(X_143,"-d\tsuppress all displays\n"));
err_printf(Str(X_147,"-g\tsuppress graphics, use ascii displays\n"));
err_printf(Str(X_107,"-G\tsuppress graphics, use Postscript displays\n"));
err_printf(Str(X_170,"-x fnam\textract from score.srt using extract file 'fnam'\n"));
err_printf(Str(X_165,"-t N\tuse uninterpreted beats of the score, initially at tempo N\n"));
err_printf(Str(X_425,"-t 0\tuse score.srt for sorted score rather than a temporary\n"));
err_printf(Str(X_112,"-L dnam\tread Line-oriented realtime score events from device 'dnam'\n"));
err_printf(Str(X_116,"-M dnam\tread MIDI realtime events from device 'dnam'\n"));
err_printf(Str(X_105,"-F fnam\tread MIDIfile event stream from file 'fnam'\n"));
/* err_printf(Str(X_121,"-P N\tMIDI sustain pedal threshold (0 - 128)\n")); */
err_printf(Str(X_125,"-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"));
err_printf(Str(X_108,"-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
err_printf(Str(X_120,"-N\tnotify (ring the bell) when score or miditrack is done\n"));
err_printf(Str(X_126,"-T\tterminate the performance when miditrack is done\n"));
err_printf(Str(X_103,"-D\tdefer GEN01 soundfile loads until performance time\n"));
#ifdef LINUX                    /* Jonathan Mohr  1995 Oct 17 */
err_printf(Str(X_1549,"-Q dnam\tselect MIDI output device\n"));
#ifdef RTAUDIO
err_printf(Str(X_130,"-V N\tset real-time audio output volume to N (1 to 100)\n"));
#endif
#endif
#ifdef __BEOS__                 /* jjk 09252000 */
err_printf(Str(X_1549,"-Q dnam\tselect MIDI output device\n"));
#endif
err_printf(Str(X_172,"-z\tList opcodes in this version\n"));
err_printf(Str(X_1537,"-Z\tDither output\n"));
err_printf(Str(X_90,"-- fnam\tlog output to file\n"));
#if defined(LINUX)
err_printf("--sched set real-time priority and lock memory\n");
err_printf("        (also requires -d and either -o dac or -o devaudio)\n");
#endif
#ifdef mills_macintosh
err_printf(Str(X_581,"_____________Macintosh Command Line Flags_________________\n"));
/* err_printf(Str(X_89,"-- fnam\t Redirect output to listing file 'fnam'\n")); */
err_printf(Str(X_133,"-X fnam\t Sound File Directory\n"));
err_printf(Str(X_160,"-q fnam\t Sound Sample-In Directory\n"));
err_printf(Str(X_123,"-Q fnam\t Analysis Directory\n"));
err_printf(Str(X_129,"-V N\t Number of chars in screen buffer for output window\n"));
err_printf(Str(X_104,"-E N\t Number of tables in graphics window\n"));
err_printf(Str(X_159,"-p\t\t Play after rendering\n"));
err_printf(Str(X_144,"-e\t\t Rescaled floats as shorts to max amplitude\n"));
err_printf(Str(X_169,"-w\t\t Record and Save MIDI input to a file\n"));
err_printf(Str(X_171,"-y N\t Enables Progress Display at rate N seconds,\n"));
err_printf(Str(X_572,"\t\t\tor for negative N, at -N kperiods\n"));
err_printf(Str(X_134,"-Y N\t Enables Profile Display at rate N in seconds,\n"));
err_printf(Str(X_572,"\t\t\tor for negative N, at -N kperiods\n"));
err_printf("-P N\t Poll Events Every N Buffer Writes\n");
err_printf(Str(X_582,"__________________________________________________________\n"));
#endif
err_printf(Str(X_768,"flag defaults: csound -s -otest -b%d -B%d -m7 -P128\n"),
        IOBUFSAMPS, IODACSAMPS);
        longjmp(cenviron.exitjmp_,1);
}

void longusage(void)
{
    err_printf(Str(X_519,"Usage:\tcsound [-flags] orchfile scorefile\n"));
    err_printf(Str(X_325,"Legal flags are:\n"));
    err_printf("Long format:\n\n");
    err_printf("--format={"
#ifdef never
               "alaw,"
#endif
#ifdef ULAW
               "ulaw,"
#endif
"schar,uchar,float,short,long,24bit,rescale}\t Set sound type\n"
"--aiff\t\t\tSet AIFF format\n"
"--wave\t\t\tSet WAV format\n"
"--ircam\t\t\tSet IRCAM format\n"
"--noheader\t\tRaw format\n"
"--nopeaks\t\tDo not write peak information\n"
"\n"
"--nodisplays\t\tsuppress all displays\n"
"--asciidisplay\t\tsuppress graphics, use ascii displays\n"
"--postscriptdisplay\tsuppress graphics, use Postscript displays\n"
"\n"
"--defer-gen1\t\tdefer GEN01 soundfile loads until performance time\n"
"--iobufsamps=N\t\tsample frames (or -kprds) per software sound I/O buffer\n"
"--hardwarebufsamps=N\tsamples per hardware sound I/O buffer\n"
"--cscore\t\tuse Cscore processing of scorefile\n"
"\n"
"--midifile=FNAME\tread MIDIfile event stream from file\n"
"--midi-device=FNAME\tread MIDI realtime events from device\n"
"--terminate-on-midi\tterminate the performance when miditrack is done\n"
"\n"
"--heartbeat=N\t\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"
"--notify\t\tnotify (ring the bell) when score or miditrack is done\n"
"--rewrite\t\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"
"\n"
"--input=FNAME\t\tsound input filename\n"
"--output=FNAME\t\tsound output filename\n"
"--logfile=FNAME\t\tlog output to file\n"
"\n"
"--nosound\t\tno sound onto disk or device\n"
"--tempo=N\t\tuse uninterpreted beats of the score, initially at tempo N\n"
"--i-only\t\tI-time only orch run\n"
"--control-rate=N\torchestra krate overrid\n"
"--sample-rate=N\t\torchestra srate override\n"
"--score-in=FNAME\tread Line-oriented realtime score events from device\n"
"--messagelevel=N\ttty message level. Sum of: 1=note amps, 2=out-of-range msg, 4=warnings\n\n"
"\n"
"--extract-score=FNAME\textract from score.srt using extract file\n"
"--keep-sorted-score\n"
"--utility=NAME\t\trun utility program\n"
"--verbose\t\tverbose orch translation\n"
"--list-opcodes\t\tList opcodes in this version\n"
"--list-opcodesN\t\tList opcodes in style N in this version\n"
"--dither\t\tDither output\n"
"--sched\t\t\tset real-time priority and lock memory\n"
"--opcode-lib=NAMES\tDynamic libraries to load\n"
"\n"
"--help\t\t\tLong help\n"
"\n"
#ifdef mills_macintosh
"--graphs=N\tNumber of tables in graphics window\n"
"--pollrate=N\n"
"--play-on-end\t Play after rendering\n"
"--sample-directory=FNAME\n"
"--analysis-directory=FNAME\n"
"--sound_directory=FNAME\n"
"--screen-buffer=N\tNumber of chars in screen buffer for output window\n"
"--save-midi\tRecord and Save MIDI input to a file\n"
#endif
);
    err_printf("\nShort form:\n");
err_printf(Str(X_128,"-U unam\trun utility program unam\n"));
err_printf(Str(X_102,"-C\tuse Cscore processing of scorefile\n"));
err_printf(Str(X_109,"-I\tI-time only orch run\n"));
err_printf(Str(X_155,"-n\tno sound onto disk\n"));
err_printf(Str(X_150,"-i fnam\tsound input filename\n"));
err_printf(Str(X_157,"-o fnam\tsound output filename\n"));
err_printf(Str(X_138,"-b N\tsample frames (or -kprds) per software sound I/O buffer\n"));
err_printf(Str(X_100,"-B N\tsamples per hardware sound I/O buffer\n"));
err_printf(Str(X_96,"-A\tcreate an AIFF format output soundfile\n"));
err_printf(Str(X_132,"-W\tcreate a WAV format output soundfile\n"));
err_printf(Str(X_111,"-J\tcreate an IRCAM format output soundfile\n"));
err_printf(Str(X_149,"-h\tno header on output soundfile\n"));
err_printf(Str(X_141,"-c\t8-bit signed_char sound samples\n"));
#ifdef never
err_printf(Str(X_136,"-a\talaw sound samples\n"));
#endif
err_printf(Str(X_94,"-8\t8-bit unsigned_char sound samples\n")); /* J. Mohr 1995 Oct 17 */
#ifdef ULAW
err_printf(Str(X_166,"-u\tulaw sound samples\n"));
#endif
err_printf(Str(X_164,"-s\tshort_int sound samples\n"));
err_printf(Str(X_153,"-l\tlong_int sound samples\n"));
err_printf(Str(X_145,"-f\tfloat sound samples\n"));
err_printf(Str(X_1138,"-3\t24bit sound samples\n"));
err_printf(Str(X_161,"-r N\torchestra srate override\n"));
err_printf(Str(X_152,"-k N\torchestra krate override\n"));
err_printf(Str(X_1552,"-K\tDo not generate PEAK chunks\n"));
err_printf(Str(X_168,"-v\tverbose orch translation\n"));
err_printf(Str(X_154,"-m N\ttty message level. Sum of: 1=note amps, 2=out-of-range msg, 4=warnings\n"));
err_printf(Str(X_143,"-d\tsuppress all displays\n"));
err_printf(Str(X_147,"-g\tsuppress graphics, use ascii displays\n"));
err_printf(Str(X_107,"-G\tsuppress graphics, use Postscript displays\n"));
err_printf(Str(X_170,"-x fnam\textract from score.srt using extract file 'fnam'\n"));
err_printf(Str(X_165,"-t N\tuse uninterpreted beats of the score, initially at tempo N\n"));
err_printf(Str(X_425,"-t 0\tuse score,srt for sorted score rather than a temporary\n"));
err_printf(Str(X_112,"-L dnam\tread Line-oriented realtime score events from device 'dnam'\n"));
err_printf(Str(X_116,"-M dnam\tread MIDI realtime events from device 'dnam'\n"));
err_printf(Str(X_105,"-F fnam\tread MIDIfile event stream from file 'fnam'\n"));
/* err_printf(Str(X_121,"-P N\tMIDI sustain pedal threshold (0 - 128)\n")); */
err_printf(Str(X_125,"-R\tcontinually rewrite header while writing soundfile (WAV/AIFF)\n"));
err_printf(Str(X_108,"-H#\tprint a heartbeat style 1, 2 or 3 at each soundfile write\n"));
err_printf(Str(X_120,"-N\tnotify (ring the bell) when score or miditrack is done\n"));
err_printf(Str(X_126,"-T\tterminate the performance when miditrack is done\n"));
err_printf(Str(X_103,"-D\tdefer GEN01 soundfile loads until performance time\n"));
#ifdef LINUX                    /* Jonathan Mohr  1995 Oct 17 */
err_printf(Str(X_1549,"-Q dnam\tselect MIDI output device\n"));
#ifdef RTAUDIO
err_printf(Str(X_130,"-V N\tset real-time audio output volume to N (1 to 100)\n"));
#endif
#endif
#ifdef __BEOS__                 /* jjk 09252000 */
err_printf(Str(X_1549,"-Q dnam\tselect MIDI output device\n"));
#endif
err_printf(Str(X_172,"-z\tList opcodes in this version\n"));
err_printf(Str(X_1537,"-Z\tDither output\n"));
err_printf(Str(X_90,"-- fnam\tlog output to file\n"));
#if defined(LINUX)
err_printf("--sched set real-time priority and lock memory\n");
err_printf("        (also requires -d and either -o dac or -o devaudio)\n");
#endif
#ifdef mills_macintosh
err_printf(Str(X_581,"_____________Macintosh Command Line Flags_________________\n"));
/* err_printf(Str(X_89,"-- fnam\t Redirect output to listing file 'fnam'\n")); */
err_printf(Str(X_133,"-X fnam\t Sound File Directory\n"));
err_printf(Str(X_160,"-q fnam\t Sound Sample-In Directory\n"));
err_printf(Str(X_123,"-Q fnam\t Analysis Directory\n"));
err_printf(Str(X_129,"-V N\t Number of chars in screen buffer for output window\n"));
err_printf(Str(X_104,"-E N\t Number of tables in graphics window\n"));
err_printf(Str(X_159,"-p\t\t Play after rendering\n"));
err_printf(Str(X_144,"-e\t\t Rescaled floats as shorts to max amplitude\n"));
err_printf(Str(X_169,"-w\t\t Record and Save MIDI input to a file\n"));
err_printf(Str(X_171,"-y N\t Enables Progress Display at rate N seconds,\n"));
err_printf(Str(X_572,"\t\t\tor for negative N, at -N kperiods\n"));
err_printf(Str(X_134,"-Y N\t Enables Profile Display at rate N in seconds,\n"));
err_printf(Str(X_572,"\t\t\tor for negative N, at -N kperiods\n"));
err_printf("-P N\t Poll Events Every N Buffer Writes\n");
err_printf(Str(X_582,"__________________________________________________________\n"));
#endif
err_printf(Str(X_768,"flag defaults: csound -s -otest -b%d -B%d -m7 -P128\n"),
        IOBUFSAMPS, IODACSAMPS);
 longjmp(cenviron.exitjmp_,0);
}

void dieu(char *s)
{
    err_printf(Str(X_236,"Csound Command ERROR:\t%s\n"),s);
    usage();
#ifdef mills_macintosh
    die("");
#else
    if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
#endif
}

typedef struct {
  char *util;
  int (*fn)(int, char**);
  int  strno;
  char *string;
} UTILS;

UTILS utilities[] = {
  { "hetro", hetro, X_1364, "util HETRO:\n" },
  { "lpanal", lpanal, X_1365, "util LPANAL:\n" },
  { "pvanal", pvanal, X_1366, "util PVANAL:\n" },
  { "sndinfo", sndinfo, X_1367, "util SNDINFO:\n" },
  { "cvanal", cvanal, X_1363, "util CVANAL:\n" },
  { "pvlook", pvlook, X_23, "util PVLOOK:\n" },
  { "dnoise", dnoise, X_1704, "util DNOISE:\n" },
  { NULL, NULL, 0}
};

void
set_output_format(char c)
{
    if (O.outformat && (O.msglevel & WARNMSG)) {
      printf(Str(X_1198,"WARNING: Sound format -%c has been overruled by -%c\n"),
             outformch, c);
    }

    switch (c) {
#ifdef never
    case 'a':
      O.outformat = AE_ALAW;    /* a-law soundfile */
      break;
#endif

    case 'c':
      O.outformat = AE_CHAR;    /* signed 8-bit soundfile */
      break;

    case '8':
      O.outformat = AE_UNCH;    /* unsigned 8-bit soundfile */
      break;

    case 'f':
      O.outformat = AE_FLOAT;   /* float soundfile */
      if (O.filetyp == TYP_AIFF) {
        if (O.msglevel & WARNMSG)
          printf(Str(X_402,
                    "WARNING: Overriding File Type to AIFF-C for float output\n"));
        O.filetyp = TYP_AIFC;
      }
      break;

    case 's':
      O.outformat = AE_SHORT;   /* short_int soundfile*/
      break;

    case 'l':
      O.outformat = AE_LONG;    /* long_int soundfile */
      break;

#ifdef ULAW
    case 'u':
      O.outformat = AE_ULAW;    /* mu-law soundfile */
      break;
#endif

    case '3':
      O.outformat = AE_24INT;   /* 24bit packed soundfile*/
      break;

    case 'e':
      O.outformat = AE_FLOAT;   /* float soundfile (for rescaling) */
      break;

    default:
      return; /* do nothing */
    };

  outformch = c;
}

typedef struct  {
  char *longformat;
  char shortformat;
} SAMPLE_FORMAT_ENTRY;

static
SAMPLE_FORMAT_ENTRY sample_format_map[] = {
#ifdef never
  {"alaw", 'a'}, 
#endif
  {"schar", 'c'}, 
#ifdef ULAW
  {"uchar", '8'}, 
#endif
  {"float", 'f'},
  {"short", 's'}, {"ulaw", 'u'}, {"24bit", '3'},
  {0, 0}
};

static int decode_long(char *s, int argc, char **argv, char *envoutyp)
{

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
            set_output_format(c);
            return 1;
          }
#ifdef mills_macintosh
        else if (!(strncmp (s, "rescale", 7))) {
          s += 7;
          set_output_format('e');
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
      if (O.filetyp == TYP_WAV) {
        if (envoutyp == NULL) goto outtyp;
        if (O.msglevel & WARNMSG)
          printf(Str(X_95,"WARNING: -A overriding local default WAV out\n"));
      }
      if (O.outformat == AE_FLOAT) {
        if (O.msglevel & WARNMSG)
          printf(Str(X_401,"WARNING: Overriding File Type to AIFF-C for AIFF float format\n"));
        O.filetyp = TYP_AIFC;
      }
      else O.filetyp = TYP_AIFF;     /* AIFF output request*/
      return 1;
    }
    else if (!(strncmp (s, "iobufsamps=", 11))) {
      s += 11;
      if (*s=='\0') dieu(Str(X_1039,"no iobufsamps"));
      /* defaults in musmon.c */
      O.inbufsamps = O.outbufsamps = atoi(s);
      return 1;
    }
    else if (!(strncmp (s, "hardwarebufsamps=", 17))) {
      s += 17;
      if (*s=='\0') dieu(Str(X_1034,"no hardware bufsamps"));
      O.inbufsamps = O.outbufsamps = atoi(s);
      return 1;
    }
    else if (!(strcmp (s, "cscore"))) {
      O.usingcscore = 1;     /* use cscore processing  */
      return 1;
    }
    else if (!(strcmp (s, "nodisplays"))) {
      O.displays = 0;           /* no func displays */
      return 1;
    }
    else if (!(strcmp (s, "defer-gen1"))) {
      O.gen01defer = 1;  /* defer GEN01 sample loads
                            until performance time */
      return 1;
    }
#ifdef mills_macintosh
    /* -E N */
    else if (!(strncmp (s, "graphs=", 7))) {
      s += 7;
      if (*s=='\0') dieu(Str(X_1049,"no number of graphs"));
      csNGraphs = atoi(s);
      SetCsNGraphs(csNGraphs);
      return 1;
    }
    else if (!(strncmp (s, "pollrate=", 9))) {
      s += 9;
      if (*s == '\0') dieu(Str(X_1551,"no poll event rate"));
      pollEventRate = atoi(s);
      return 1;
    }
    else if (!(strcmp(s, "play-on-end"))) {
      SetPlayOnFinish(TRUE);
      return 1;
    }
    else if (!(strncmp (s, "sample-directory=", 17))) {
      s += 17;
      if (*s == '\0') dieu(Str(X_1060,"no sound sample directory name")) ;
      strcpy(ssdir_path,s);
      return 1;
    }
    else if (!(strncmp (s, "analysis-directory=", 19))) {
      s += 19;
      if (*s == '\0') dieu(Str(X_1023,"no analysis directory name")) ;
      strcpy(sadir_path,s);
      return 1;
    }
#endif
    else if (!(strncmp (s, "midifile=", 9))) {
      s += 9;
      if (*s == '\0') dieu(Str(X_1048,"no midifile name"));
      O.FMidiname = s;    /* Midifile name */
      if (!strcmp(O.FMidiname,"stdin")) {
        if (stdinassgn)
          dieu(Str(X_106,"-F: stdin previously assigned"));
        stdinassgn = 1;
      }
      O.FMidiin = 1;          /***************/
    }
    /* -g */
    else if (!(strcmp (s, "asciidisplay"))) {
      O.graphsoff = 1;          /* don't use graphics but ASCII */
      return 1;
    }
    /* -G */
    else if (!(strcmp (s, "postscriptdisplay"))) {
      O.postscript = 1;        /* don't use graphics but PostScript */
      return 1;
    }
    /* -h */
    else if (!(strcmp (s, "noheader"))) {
      O.sfheader = 0;          /* skip sfheader  */
      return 1;
    }
    else if (!(strncmp (s, "heartbeat=", 10))) {
      s += 10;
      if (*s == '\0') O.heartbeat = 1;
      else O.heartbeat = atoi(s);
      return 1;
    }
#ifdef EMBEDDED_PYTHON
    else if (strncmp(s, "pyvar=", 6) == 0)
      {
        s += 6;
        if (python_add_cmdline_definition(s))
          dieu("invalid python variable definition syntax");
        return 1;
      }
#endif
    else if (!(strncmp (s, "input=", 6))) {
      s += 6;
      if (*s == '\0') dieu(Str(X_1038,"no infilename"));
      O.infilename = s;   /* soundin name */
      if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
      if (strcmp(O.infilename,"stdout") == 0)
        dieu("input cannot be stdout");
      if (strcmp(O.infilename,"stdin") == 0)
#if defined mills_macintosh || defined SYMANTEC
        dieu(Str(X_1242,"stdin audio not supported"));
#else
      {
        if (stdinassgn)
          dieu(Str(X_151,"-i: stdin previously assigned"));
        stdinassgn = 1;
      }
#endif
      O.sfread = 1;
      return 1;
    }
    /*
-I I-time only orch run
    */
    else if (!(strcmp (s, "i-only"))) {
      O.initonly = 1;
      return 1;
    }
    /*
-j Used in localisation
-J create an IRCAM format output soundfile
    */
    else if (!(strcmp (s, "ircam"))) {
      if (O.filetyp == TYP_AIFF ||
          O.filetyp == TYP_WAV) {
        if (envoutyp == NULL) goto outtyp;
        if (O.msglevel & WARNMSG)
          printf(Str(X_110,"WARNING: -J overriding local default AIFF/WAV out\n"));
      }
      O.filetyp = TYP_IRCAM;      /* IRCAM output request */
      return 1;
    }
    /*
-k N orchestra krate override
    */
    else if (!(strncmp(s, "control-rate=", 13))) {
      s += 13;
      if (*s=='\0') dieu(Str(X_1029,"no control rate"));
      O.kr_override = atoi(s);
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
      if (*s=='\0') dieu(Str(X_1017,"no Linein score device_name"));
      O.Linename = s;
      if (!strcmp(O.Linename,"stdin")) {
        if (stdinassgn)
          dieu(Str(X_115,"-L: stdin previously assigned"));
        stdinassgn = 1;
      }
      O.Linein = 1;
      return 1;
    }
    /*
-m N tty message level. Sum of: 1=note amps, 2=out-of-range msg, 4=warnings
    */
    else if (!(strncmp (s, "messagelevel=", 13))) {
      s += 13;
      if (*s=='\0') dieu(Str(X_1046,"no message level"));
      O.msglevel = atoi(s);
      return 1;
    }
    /*
-M dnam read MIDI realtime events from device 'dnam'
    */
    else if (!(strncmp (s, "midi-device=", 12))) {
      s += 12;
      if (*s=='\0') dieu(Str(X_1047,"no midi device_name"));
      O.Midiname = s;
      if (!strcmp(O.Midiname,"stdin")) {
        if (stdinassgn)
          dieu(Str(X_119,"-M: stdin previously assigned"));
        stdinassgn = 1;
      }
      O.Midiin = 1;
      return 1;
    }
    /* -n no sound */
    else if (!(strcmp (s, "nosound"))) {
      O.sfwrite = 0;            /* nosound        */
      return 1;
    }
    /* -N */
    else if (!(strcmp (s, "notify"))) {
      O.ringbell = 1;        /* notify on completion */
      return 1;
    }
    else if (!(strncmp (s, "output=", 7))) {
      s += 7;
      if (*s == '\0') dieu(Str(X_1052,"no outfilename"));
      O.outfilename = s;          /* soundout name */
      if (strcmp(O.outfilename,"stdin") == 0)
        dieu(Str(X_156,"-o cannot be stdin"));
      if (strcmp(O.outfilename,"stdout") == 0)
#if defined mac_classic || defined SYMANTEC || defined __WATCOMC__ || defined WIN32
        dieu(Str(X_1244,"stdout audio not supported"));
#else
      {
        if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
          die(Str(X_1290,"too many open files"));
        dup2(2,1);                /* & send 1's to stderr */
      }
#endif
      return 1;
    }
    else if (!(strncmp (s, "logfile=", 8))) {
      s += 8;
      if (*s=='\0') dieu(Str(X_1044,"no log file"));
      dribble = fopen(s, "w");
      return 1;
    }
  /* -r N */
    else if (!(strncmp (s, "sample-rate=", 12))) {
      s += 12;
      O.sr_override = atol(s);
      return 1;
    }
    /* R */
    else if (!(strcmp (s, "rewrite"))) {
      O.rewrt_hdr = 1;
      return 1;
    }
    /* -S  */
    /* tempo=N use uninterpreted beats of the score, initially at tempo N
    */
    else if (!(strncmp (s, "tempo=", 6))) {
    s += 6;
    O.cmdTempo = atoi(s);
    O.Beatmode = 1;       /* on uninterpreted Beats */
      return 1;
    }
    /* -t0 */
    else if (!(strcmp (s, "keep-sorted-score"))) {
      keep_tmp = 1;
      return 1;
    }
    /* -T terminate the performance when miditrack is done */
    else if (!(strcmp (s, "terminate-on-midi"))) {
            O.termifend = 1;       /* terminate on midifile end */
      return 1;
    }
    else if (!(strncmp (s, "utility=", 8))) {
      int n;
      s += 8;
      if (*s=='\0') dieu(Str(X_1064,"no utility name"));
#ifdef mills_macintosh
      util_perf = true;
      transport.state |= kUtilPerf;
#endif
      for (n=0; utilities[n].util!=NULL; n++) {
        if (strcmp(s,utilities[n].util) == 0) {
          printf(Str(utilities[n].strno, utilities[n].string));
#ifdef mills_macintosh
          SIOUXSetTitle((unsigned char *)CtoPstr((char *)s));
#endif
          (utilities[n].fn)(argc,argv);
          return 0;
        }
      }
      dies(Str(X_127,"-U %s not a valid UTIL name"),s);
      return 0;
    }
    /* -v */
    else if (!(strcmp (s, "verbose"))) {
      O.odebug = 1;    /* verbose otran  */
      return 1;
    }
#ifdef mills_macintosh
    /* -V N  Number of chars in screen buffer for output window */
    else if (!(strncmp(s, "screen-buffer=", 14))) {
      s += 14;
      if (*s=='\0') dieu(Str(X_1058,"no screen buffer size"));
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
      if (*s=='\0') dieu(Str(X_1068,"no xfilename"));
      xfilename = s;
      return 1;
    }
#ifdef mills_macintosh
    /* -X fnam  Sound File Directory */
    else if (!(strncmp(s, "sound_directory=", 16))) {
      s += 16;
      if (*s=='\0') dieu(Str(X_1059,"no sound file directory name"));
      strcpy(sadir_path,s);
      return 1;
    }
    /* -y N  Enables Profile Display at rate N in seconds,
       or for negative N, at -N kperiods
    else if (!(strncmp(s, "progress-rate=", 14))) {
      s += 14;
      if (*s=='\0') dieu(Str(X_1020,"no Rate for Progress Display"));
      err_printf(
      Str(X_488,"The Progress/Profile feature is currently disabled, sorry.\n"));
      return 1;
    }
    */
    /* -Y N  Enables Progress Display at rate N seconds,
   or for negative N, at -N kperiods
    else if (!(strncmp(s, "profile-rate=", 13))) {
      s += 13;
      if (*s=='\0') dieu(Str(X_1019,"no Rate for Profile Display"));
      return 1;
    }
    */
#endif
    else if (!(strcmp(s, "wave"))) {
      if (O.filetyp == TYP_AIFF) {
        if (envoutyp == NULL) goto outtyp;
        if (O.msglevel & WARNMSG)
          printf(Str(X_131,"WARNING: -W overriding local default AIFF out\n"));
      }
      if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
      O.filetyp = TYP_WAV;      /* WAV output request */
      return 1;
    }
    else if (!(strcmp (s, "list-opcodes"))) {
      int full = 0;
      if (*s != '\0') {
        if (isdigit(*s)) full = *s++ - '0';
      }
      create_opcodlst();
      list_opcodes(full);
#ifndef mills_macintosh
      return (0);
#else
      return 1;
#endif
    }
    /* -Z */
    else if (!(strcmp (s, "dither"))) {
      dither_output = 1;
      return 1;
    }
    else if (!(strncmp(s, "opcode-lib=", 11))) {
      s += 11;
      cenviron.oplibs_ = s;
      return 1;
    }
    else if (!(strcmp(s ,"help"))) {
      longusage();
      return 0;
    }
outtyp:
    return (0);
}

int argdecode(int argc, char **argv, char **pfilnamp, char *envoutyp)
{
    char *s;
    char c;
    int n;
    char *filnamp = *pfilnamp;

#ifdef mills_macintosh
         keep_tmp = 1;
 #else
    keep_tmp = 0;
#endif

    do {

      s = *++argv;
      if (*s++ == '-') {                        /* read all flags:  */
        while ((c = *s++) != '\0') {
          switch(c) {
          case 'U':
            FIND(Str(X_1064,"no utility name"));
#ifdef mills_macintosh
            util_perf = true;
            transport.state |= kUtilPerf;
#endif
            for (n=0; utilities[n].util!=NULL; n++) {
              if (strcmp(s,utilities[n].util) == 0) {
              printf(Str(utilities[n].strno, utilities[n].string));
#ifdef mills_macintosh
              SIOUXSetTitle((unsigned char *)CtoPstr((char *)s));
#endif
              (utilities[n].fn)(argc,argv);
              goto fnd;
              }
            }
            dies(Str(X_127,"-U %s not a valid UTIL name"),s);
          fnd:
            *pfilnamp = filnamp;
            return(0);
            /********** commandline flags only for mac version***************/
            /*********************  matt 5/26/96 ****************************/
#ifdef mills_macintosh
          case 'q':
            FIND(Str(X_1060,"no sound sample directory name")) ;
            foo = filnamp;
            while ((*filnamp++ = *s++));  s--;
            strcpy(ssdir_path,foo);
            break;
          case 'Q':
            FIND(Str(X_1023,"no analysis directory name")) ;
            foo = filnamp;
            while ((*filnamp++ = *s++));  s--;
            strcpy(sadir_path,foo);
            break;
          case 'X':
            FIND(Str(X_1059,"no sound file directory name"));
            foo = filnamp;
            while ((*filnamp++ = *s++));  s--;
            strcpy(sfdir_path,foo);
            break;
/*      case '-': FIND(Str(X_1043,"no listing file name")) ;
            foo = filnamp;
            while ((*filnamp++ = *s++));  s--;
            strcpy(listing_file,foo);
            printf(Str(X_1158,"redirecting standard out to %s......\n"),
                   listing_file);
            if (!freopen(listing_file,"w",stdout))
                          die(Str(X_673,"could not redirect sandard out\n"));
            break;
*/
          case 'V':
            FIND(Str(X_1058,"no screen buffer size"));
            sscanf(s,"%d",&vbuf);
            fflush(stdout);
            setvbuf(stdout, NULL, _IOFBF, vbuf);
            while (*++s);
            break;
          case 'P':
            FIND(Str(X_1551,"no poll event rate"));
            sscanf(s,"%d",&n);
            while (*++s);
            pollEventRate = n;
            break;
          case 'E':
            FIND(Str(X_1049,"no number of graphs"));
            sscanf(s,"%d",&csNGraphs);
            SetCsNGraphs(csNGraphs);
            while (*++s);
            break;
          case 'p':
            SetPlayOnFinish(TRUE);
            break;
          case 'e':
            set_output_format('e');
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
            O.usingcscore = 1;     /* use cscore processing  */
            break;
          case 'I':
            O.initonly = 1;           /* I-only implies */
          case 'n':
            O.sfwrite = 0;            /* nosound        */
            break;
          case 'i':
            FIND(Str(X_1038,"no infilename"));
            O.infilename = filnamp;   /* soundin name */
            while ((*filnamp++ = *s++));  s--;
            if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
            if (strcmp(O.infilename,"stdout") == 0)
              dieu("-i cannot be stdout");
            if (strcmp(O.infilename,"stdin") == 0)
#if defined mills_macintosh || defined SYMANTEC
              dieu(Str(X_1242,"stdin audio not supported"));
#else
            {
              if (stdinassgn)
                dieu(Str(X_151,"-i: stdin previously assigned"));
              stdinassgn = 1;
            }
#endif
            O.sfread = 1;
            break;
          case 'o':
            FIND(Str(X_1052,"no outfilename"));
            O.outfilename = filnamp;          /* soundout name */
            while ((*filnamp++ = *s++)); s--;
            if (strcmp(O.outfilename,"stdin") == 0)
              dieu(Str(X_156,"-o cannot be stdin"));
            if (strcmp(O.outfilename,"stdout") == 0)
#if defined mac_classic || defined SYMANTEC || defined BCC || defined __WATCOMC__ || defined WIN32
              dieu(Str(X_1244,"stdout audio not supported"));
#else
            {
              if ((O.stdoutfd = dup(1)) < 0) /* redefine stdout */
                die(Str(X_1290,"too many open files"));
              dup2(2,1);                /* & send 1's to stderr */
            }
#endif
            break;
          case 'b':
            FIND(Str(X_1039,"no iobufsamps"));
            sscanf(s,"%d%n",&O.outbufsamps, &n);
            /* defaults in musmon.c */
            O.inbufsamps = O.outbufsamps;
            s += n;
            break;
          case 'B':
            FIND(Str(X_1034,"no hardware bufsamps"));
            sscanf(s,"%d%n",&O.oMaxLag, &n);
            /* defaults in rtaudio.c */
            s += n;
            break;
          case 'A':
            if (O.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str(X_95,"WARNING: -A overriding local default WAV out\n"));
            }
            if (O.outformat == AE_FLOAT) {
              if (O.msglevel & WARNMSG)
                printf(Str(X_401,
                          "WARNING: Overriding File Type to AIFF-C for AIFF float format\n"));
              O.filetyp = TYP_AIFC;
            }
            else O.filetyp = TYP_AIFF;     /* AIFF output request*/
            break;
          case 'J':
            if (O.filetyp == TYP_AIFF ||
                O.filetyp == TYP_WAV) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str(X_110,"WARNING: -J overriding local default AIFF/WAV out\n"));
            }
            O.filetyp = TYP_IRCAM;      /* IRCAM output request */
            break;
          case 'W':
            if (O.filetyp == TYP_AIFF) {
              if (envoutyp == NULL) goto outtyp;
              if (O.msglevel & WARNMSG)
                printf(Str(X_131,"WARNING: -W overriding local default AIFF out\n"));
            }
            if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
            O.filetyp = TYP_WAV;      /* WAV output request */
            break;
          case 'h':
            O.sfheader = 0;           /* skip sfheader  */
            break;
          case 'c':
          case 'a':
          case 'u':
          case '8':
          case 's':
          case '3':
          case 'l':
          case 'f':
            set_output_format(c);
            break;
          case 'r':
            FIND(Str(X_1056,"no sample rate"));
            sscanf(s,"%ld",&O.sr_override);
            while (*++s);
            break;
          case 'j':
            FIND("");
            while (*++s);
            break;
          case 'k':
            FIND(Str(X_1029,"no control rate"));
            sscanf(s,"%ld",&O.kr_override);
            while (*++s);
            break;
          case 'v':
            O.odebug = 1;    /* verbose otran  */
            break;
          case 'm':
            FIND(Str(X_1046,"no message level"));
            sscanf(s,"%d%n",&O.msglevel, &n);
            s += n;
            break;
          case 'd':
            O.displays = 0;           /* no func displays */
            break;
          case 'g':
            O.graphsoff = 1;          /* don't use graphics */
            break;
          case 'G':
            O.postscript = 1;         /* Postscript graphics*/
            break;
          case 'x':
            FIND(Str(X_1068,"no xfilename"));
            xfilename = s;            /* extractfile name */
            while (*++s);
            break;
          case 't':
            FIND(Str(X_1063,"no tempo value"));
            {
              int val;
              sscanf(s,"%d%n",&val, &n);/* use this tempo .. */
              s += n;
              if (val < 0) dieu(Str(X_890,"illegal tempo"));
              else if (val == 0) {
                keep_tmp = 1;
                break;
              }
              else O.cmdTempo = val;
              O.Beatmode = 1;       /* on uninterpreted Beats */
            }
            break;
          case 'L':
            FIND(Str(X_1017,"no Linein score device_name"));
            O.Linename = filnamp;     /* Linein device name */
            while ((*filnamp++ = *s++));  s--;
            if (!strcmp(O.Linename,"stdin")) {
              if (stdinassgn)
                dieu(Str(X_115,"-L: stdin previously assigned"));
              stdinassgn = 1;
            }
            O.Linein = 1;
            break;
          case 'M':
            FIND(Str(X_1047,"no midi device_name"));
            O.Midiname = filnamp;     /* Midi device name */
            while ((*filnamp++ = *s++));  s--;
            if (!strcmp(O.Midiname,"stdin")) {
              if (stdinassgn)
                dieu(Str(X_119,"-M: stdin previously assigned"));
              stdinassgn = 1;
            }
            O.Midiin = 1;
            break;
          case 'F':
            FIND(Str(X_1048,"no midifile name"));
            O.FMidiname = filnamp;    /* Midifile name */
            while ((*filnamp++ = *s++));  s--;
            if (!strcmp(O.FMidiname,"stdin")) {
              if (stdinassgn)
                dieu(Str(X_106,"-F: stdin previously assigned"));
              stdinassgn = 1;
            }
            O.FMidiin = 1;          /***************/
            break;
#ifdef LINUX
          case 'Q':
            FIND(Str(X_1018,"no MIDI output device"));
            midi_out = -1;
            if (isdigit(*s)) {
              sscanf(s,"%d%n",&midi_out,&n);
              s += n;
              openMIDIout();
            }
            break;
#endif
#ifdef __BEOS__                     /* jjk 09252000 - MIDI output device */
          case 'Q':
            FIND(Str(X_1550,"no midi output device name"));
            O.Midioutname = filnamp;
            while ((*filnamp++ = *s++));  s--;
            break;
#endif
          case 'R':
            O.rewrt_hdr = 1;
            break;
          case 'H':
            if (isdigit(*s)) {
              sscanf(s, "%d%n", &O.heartbeat, &n);
              s += n;
            }
            else O.heartbeat = 1;
            break;
          case 'N':
            O.ringbell = 1;        /* notify on completion */
            break;
          case 'T':
            O.termifend = 1;       /* terminate on midifile end */
            break;
          case 'D':
            O.gen01defer = 1;  /* defer GEN01 sample loads
                                  until performance time */
            break;
          case 'K':
            peakchunks = 0;     /* Do not write peak information */
            break;
#ifdef LINUX
#ifdef RTAUDIO
          /* Add option to set soundcard output volume for real-
                       time audio output under Linux. -- J. Mohr 95 Oct 17 */
          case 'V':
            FIND(Str(X_1066,"no volume level"));
            sscanf(s,"%d%n",&O.Volume, &n);
            s += n;
            break;
#endif
#endif
          case 'z':
            {
              int full = 0;
              if (*s != '\0') {
                if (isdigit(*s)) full = *s++ - '0';
              }
              create_opcodlst();
              list_opcodes(full);
            }
#ifndef mills_macintosh
            *pfilnamp = filnamp;
            return (1);
#else
            break;
#endif
          case 'Z':
            dither_output = 1;
            break;
          case '@':
            FIND("No indirection file");
            {
              FILE *ind = fopen(s, "r");
              if (ind==0) {
                sprintf(errmsg, "Can not open indirection file %s\n", s);
                dieu(errmsg);
              }
              else {
                readOptions(ind);
                fclose(ind);
              }
              while (*s++); s--;
            }
            break;
          case 'O':
            FIND(Str(X_1044,"no log file"));
            dribble = fopen(s, "w");
            while (*s++); s--;
            break;
          case '-':
#if defined(LINUX)
            if (!(strcmp (s, "sched"))) {           /* ignore --sched */
              while (*(++s));
              break;
            }
#endif
            if (!decode_long(s, argc, argv, envoutyp))
              goto outtyp;
            while (*(++s));
            break;
          default:
            sprintf(errmsg,Str(X_1334,"unknown flag -%c"), c);
            dieu(errmsg);
          }
          if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
        }
      }
      else {
        if (orchname == NULL)
          orchname = --s;
        else if (scorename == NULL)
          scorename = --s;
        else {
          err_printf("argc=%d Additional string \"%s\"\n", argc, --s);
          dieu(Str(X_1286,"too many arguments"));
        }
      }
      if (!POLL_EVENTS()) longjmp(cenviron.exitjmp_,1);
    } while (--argc);
    *pfilnamp = filnamp;
    return 1;

outtyp:
    dieu(Str(X_1113,"output soundfile cannot be both AIFF and WAV"));

    *pfilnamp = filnamp;
    return (0);
}

void argdecodeRESET(void)
{
    stdinassgn = 0;
}
