/*
  csound_files.h: file open utilities

  Copyright (C) 2024 V Lazzarini

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

#ifndef CSOUND_FILES_H
#define CSOUND_FILES_H

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Soundfile interface data
   */
  typedef struct sflib_info {
    long  frames ;     
    int32_t   samplerate ;
    int32_t   channels ;
    int32_t   format ;
  } SFLIB_INFO;
 
  /** 
   * Soundfile interface callbacks
   */
  typedef struct sndfileCallbacks {
    void *(*sndfileOpen)(CSOUND *csound, const char *path, int32_t mode,
                        SFLIB_INFO *sfinfo);
    void *(*sndfileOpenFd)(CSOUND *csound,
                          int32_t fd, int32_t mode, SFLIB_INFO *sfinfo,
                          int32_t close_desc);
    int32_t (*sndfileClose)(CSOUND *csound, void *);
    int64_t (*sndfileWrite)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*sndfileRead)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*sndfileWriteSamples)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*sndfileReadSamples)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*sndfileSeek)(CSOUND *, void *, int64_t, int32_t);
    int32_t (*sndfileSetString)(CSOUND *csound, void *sndfile, int32_t str_type, const char* str);
    const char *(*sndfileStrError)(CSOUND *csound, void *);
    int32_t (*sndfileCommand)(CSOUND *, void *, int32_t , void *, int32_t );
  } SNDFILE_CALLBACKS;
  

/**
   * The following constants are used with csound->FileOpen() and
   * csound->ldmemfile() to specify the format of a file that is being
   * opened.  This information is passed by Csound to a host's FileOpen
   * callback and does not influence the opening operation in any other
   * way. Conversion from Csound's TYP_XXX macros for audio formats to
   * CSOUND_FILETYPES values can be done with csound->type2csfiletype().
   */
  typedef enum {
    CSFTYPE_UNIFIED_CSD = 1,   /* Unified Csound document */
    CSFTYPE_ORCHESTRA,         /* the primary orc file (may be temporary) */
    CSFTYPE_SCORE,             /* the primary sco file (may be temporary)
                                  or any additional score opened by Cscore */
    CSFTYPE_ORC_INCLUDE,       /* a file #included by the orchestra */
    CSFTYPE_SCO_INCLUDE,       /* a file #included by the score */
    CSFTYPE_SCORE_OUT,         /* used for score.srt, score.xtr, cscore.out */
    CSFTYPE_SCOT,              /* Scot score input format */
    CSFTYPE_OPTIONS,           /* for .csoundrc and -@ flag */
    CSFTYPE_EXTRACT_PARMS,     /* extraction file specified by -x */

    /* audio file types that Csound can write (10-19) or read */
    CSFTYPE_RAW_AUDIO,
    CSFTYPE_IRCAM,
    CSFTYPE_AIFF,
    CSFTYPE_AIFC,
    CSFTYPE_WAVE,
    CSFTYPE_AU,
    CSFTYPE_SD2,
    CSFTYPE_W64,
    CSFTYPE_WAVEX,
    CSFTYPE_FLAC,
    CSFTYPE_CAF,
    CSFTYPE_WVE,
    CSFTYPE_OGG,
    CSFTYPE_MPC2K,
    CSFTYPE_RF64,
    CSFTYPE_AVR,
    CSFTYPE_HTK,
    CSFTYPE_MAT4,
    CSFTYPE_MAT5,
    CSFTYPE_NIST,
    CSFTYPE_PAF,
    CSFTYPE_PVF,
    CSFTYPE_SDS,
    CSFTYPE_SVX,
    CSFTYPE_VOC,
    CSFTYPE_XI,
    CSFTYPE_MPEG,
    CSFTYPE_UNKNOWN_AUDIO,     /* used when opening audio file for reading
                                  or temp file written with <CsSampleB> */

    /* miscellaneous music formats */
    CSFTYPE_SOUNDFONT,
    CSFTYPE_STD_MIDI,          /* Standard MIDI file */
    CSFTYPE_MIDI_SYSEX,        /* Raw MIDI codes, eg. SysEx dump */

    /* analysis formats */
    CSFTYPE_HETRO,
    CSFTYPE_HETROT,
    CSFTYPE_PVC,               /* original PVOC format */
    CSFTYPE_PVCEX,             /* PVOC-EX format */
    CSFTYPE_CVANAL,
    CSFTYPE_LPC,
    CSFTYPE_ATS,
    CSFTYPE_LORIS,
    CSFTYPE_SDIF,
    CSFTYPE_HRTF,

    /* Types for plugins and the files they read/write */
    CSFTYPE_UNUSED,
    CSFTYPE_LADSPA_PLUGIN,
    CSFTYPE_SNAPSHOT,

    /* Special formats for Csound ftables or scanned synthesis
       matrices with header info */
    CSFTYPE_FTABLES_TEXT,        /* for ftsave and ftload  */
    CSFTYPE_FTABLES_BINARY,      /* for ftsave and ftload  */
    CSFTYPE_XSCANU_MATRIX,       /* for xscanu opcode  */

    /* These are for raw lists of numbers without header info */
    CSFTYPE_FLOATS_TEXT,         /* used by GEN23, GEN28, dumpk, readk */
    CSFTYPE_FLOATS_BINARY,       /* used by dumpk, readk, etc. */
    CSFTYPE_INTEGER_TEXT,        /* used by dumpk, readk, etc. */
    CSFTYPE_INTEGER_BINARY,      /* used by dumpk, readk, etc. */

    /* image file formats */
    CSFTYPE_IMAGE_PNG,

    /* For files that don't match any of the above */
    CSFTYPE_POSTSCRIPT,          /* EPS format used by graphs */
    CSFTYPE_SCRIPT_TEXT,         /* executable script files (eg. Python) */
    CSFTYPE_OTHER_TEXT,
    CSFTYPE_OTHER_BINARY,

    /* This should only be used internally by the original FileOpen()
       API call or for temp files written with <CsFileB> */
    CSFTYPE_UNKNOWN = 0
  } CSOUND_FILETYPES;


    /**
   * Sets an external callback for opening a sound file.
   * The callback is made when a sound file is going to be opened.
   * The following information is passed to the callback:
   *     char*  pathname of the file; either full or relative to current dir
   *     int32_t    flags of the file descriptor.
   *     SFLIB_INFO* sound file info of the sound file.
   *
   * Pass NULL to disable the callback.
   * This callback is retained after a csoundReset() call.
   */

  PUBLIC void csoundSetOpenSoundFileCallback(CSOUND *p,
                                             void *(*openSoundFileCallback)(CSOUND*,
                                                                            const char*,
                                                                            int32_t, void*));

  /**
   * Sets an external callback for opening a file.
   * The callback is made when a file is going to be opened.
   * The following information is passed to the callback:
   *     char*  pathname of the file; either full or relative to current dir
   *     char*  access mode of the file.
   *
   * Pass NULL to disable the callback.
   * This callback is retained after a csoundReset() call.
   */
  PUBLIC void csoundSetOpenFileCallback(CSOUND *p,
                                        FILE *(*openFileCallback)(CSOUND*,
                                                                  const char*,
                                                                  const char*));

#if !defined(SWIG)
  /**
   * Sets an external callback for receiving notices whenever Csound opens
   * a file.  The callback is made after the file is successfully opened.
   * The following information is passed to the callback:
   *     char*  pathname of the file; either full or relative to current dir
   *     int32_t    a file type code from the enumeration CSOUND_FILETYPES
   *     int32_t    1 if Csound is writing the file, 0 if reading
   *     int32_t    1 if a temporary file that Csound will delete; 0 if not
   *
   * Pass NULL to disable the callback.
   * This callback is retained after a csoundReset() call.
   */
  PUBLIC void csoundSetFileOpenCallback(CSOUND *p,
                                        void (*func)(CSOUND*, const char*,
                                                     int32_t, int32_t, int32_t));

  /** 
   * Sets external callbacks for the soundfile interface
   * replacing any default callbacks. These functions are used by
   * Csound to perform soundfile IO throughout the system.
   * These are set to sndfile.h functions if USE_LIBSNDFILE is defined
   * otherwise they are set to non-op stubs.
   * Callbacks are passed via the SNDFILE_CALLBACKS struct.
   * Any NULL callback pointer will disable setting of the specific callback
   * If p is NULL then all callbacks are reverted to default values.
   **/
  PUBLIC void csoundSetSndfileCallbacks(CSOUND *csound, SNDFILE_CALLBACKS *p);
  
#endif

#ifdef __cplusplus
}
#endif

#endif // CSOUND_FILES_H
