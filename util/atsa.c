/*
    atsa.c:

    ATS analysis utility
    Copyright (C) 2002-2004 Oscar Pablo Di Liscia, Pete Moss, Juan Pampin
    Ported to Csound by Istvan Varga, original version is available at
      http://sourceforge.net/projects/atsa/

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

#define _FILE_OFFSET_BITS 64

#include "std_util.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__GNUC__) && defined(__STRICT_ANSI__)
#  ifndef inline
#    define inline  __inline__
#  endif
#endif

typedef MYFLT mus_sample_t;

/*  window types */
#define  BLACKMAN   0
#define  BLACKMAN_H 1
#define  HAMMING    2
#define  VONHANN    3

/********** ANALYSIS PARAMETERS ***********/
/* start time */
#define  ATSA_START 0.0f
/* duration */
#define  ATSA_DUR 0.0f
/* lowest frequency (hertz)  */
#define  ATSA_LFREQ 20.0f
/* highest frequency (hertz) */
#define  ATSA_HFREQ 20000.0f
/* frequency deviation (ratio) */
#define  ATSA_FREQDEV 0.1f
/* number of f0 cycles in window */
#define  ATSA_WCYCLES 4
/* window type */
#define  ATSA_WTYPE BLACKMAN_H
/* window size */
#define  ATSA_WSIZE 1024
/* hop size proportional to window size (ratio) */
#define  ATSA_HSIZE 0.25f
/* lowest magnitude for peaks (amp) */
#define  ATSA_LMAG  -60.0f
/* length of analysis tracks (frames) */
#define  ATSA_TRKLEN 3
/* minimum short partial length (frames) */
#define  ATSA_MSEGLEN 3
/* minimum short partial SMR avreage (dB SPL) */
#define  ATSA_MSEGSMR 60.0f
/* minimum gap length (frames) */
#define  ATSA_MGAPLEN 3
/* threshold for partial SMR average (dB SPL) */
#define  ATSA_SMRTHRES 30.0f
/* last peak contribution for tracking (ratio) */
#define  ATSA_LPKCONT 0.0f
/* SMR contribution for tracking (ratio) */
#define  ATSA_SMRCONT 0.5f
/* minimum number of frames for analysis (frames) */
#define ATSA_MFRAMES 4
/* default analysis file type
 * 1 =only amp. and freq.
 * 2 =amp., freq. and phase
 * 3 =amp., freq. and noise
 * 4 =amp., freq., phase, and noise
 */
#define ATSA_TYPE 4
/* default residual file */
#if defined(LINUX) || defined(MACOSX)
#  define ATSA_RES_FILE "/tmp/atsa_res.wav"
#else
#  define ATSA_RES_FILE "/atsa_res.wav"
#endif

/* constants and macros */
#define  NIL                    (-1)
#define  ATSA_MAX_DB_SPL        (100.0)
#define  ATSA_NOISE_THRESHOLD   (-120)
#define  ATSA_CRITICAL_BANDS    (25)
#define  ATSA_NOISE_VARIANCE    (0.04)
/* array of critical band frequency edges base on data from:
 * Zwicker, Fastl (1990) "Psychoacoustics Facts and Models",
 * Berlin ; New York : Springer-Verlag
 */
#define ATSA_CRITICAL_BAND_EDGES {0.0, 100.0, 200.0, 300.0, 400.0, 510.0,   \
                                  630.0, 770.0, 920.0, 1080.0, 1270.0,      \
                                  1480.0, 1720.0, 2000.0, 2320.0, 2700.0,   \
                                  3150.0, 3700.0, 4400.0, 5300.0, 6400.0,   \
                                  7700.0, 9500.0, 12000.0, 15500.0, 20000.0}

//#define  AMP_DB(amp)  ((amp) != 0.0 ? (float) log10((amp) * 20.0) : -32767.0f)
//#define  DB_AMP(db)   ((float) pow(10.0, (db) / 20.0))

/* data structures */

/* ANARGS
 * ======
 * analysis parameters
 */
typedef struct {
    /* args[0] is infile, args[1] is outfile */
    char    *args[2];
    float   start;
    float   duration;
    float   lowest_freq;
    float   highest_freq;
    float   freq_dev;
    int32_t     win_cycles;
    int32_t     win_type;
    int32_t     win_size;
    float   hop_size;
    float   lowest_mag;
    int32_t     track_len;
    int32_t     min_seg_len;
    int32_t     min_gap_len;
    float   last_peak_cont;
    float   SMR_cont;
    float   SMR_thres;
    float   min_seg_SMR;
    /* parameters computed from command line */
    int32_t     first_smp;
    int32_t     cycle_smp;
    int32_t     hop_smp;
    int32_t     total_samps;
    int32_t     srate;
    int32_t     fft_size;
    float   fft_mag;
    int32_t     lowest_bin;
    int32_t     highest_bin;
    int32_t     frames;
    int32_t     type;
} ANARGS;

/* ATS_FFT
 * fft data
 */

typedef struct {
    int32_t     size;
    int32_t     rate;
    MYFLT   *data;
} ATS_FFT;

/* ATS_PEAK
 * ========
 * spectral peak data
 */
typedef struct {
    double  amp;
    double  frq;
    double  pha;
    double  smr;
    int32_t     track;
} ATS_PEAK;

/* ATS_FRAME
 * =========
 * analysis frame data
 */
typedef struct {
    ATS_PEAK *peaks;
    int32_t     n_peaks;
    double  time;
} ATS_FRAME;

/* ATS_HEADER
 * ==========
 * ats file header data
 */
typedef struct {
    /* Magic Number for ID of file, must be 123.00 */
    double  mag;
    /* sampling rate */
    double  sr;
    /* Frame size (samples) */
    double  fs;
    /* Window size (samples) */
    double  ws;
    /* number of partials per frame */
    double  par;
    /* number of frames present */
    double  fra;
    /* max. amplitude */
    double  ma;
    /* max. frequency */
    double  mf;
    /* duration (secs) */
    double  dur;
    /* type (1,2 3 or 4)
     * 1 =only amp. and freq.
     * 2 =amp., freq. and phase
     * 3 =amp., freq. and noise
     * 4 =amp., freq., phase, and noise
     */
    double  typ;
} ATS_HEADER;

/* ATS_SOUND
 * =========
 * ats analysis data
 */
typedef struct {
    /* global sound info */
    int32_t     srate;
    int32_t     frame_size;
    int32_t     window_size;
    int32_t     partials;
    int32_t     frames;
    double  dur;
    /* info deduced from analysis */
    /* we use optimised to keep the
     * # of partials killed by optimisation
     */
    int32_t     optimized;
    double  ampmax;
    double  frqmax;
    ATS_PEAK *av;
    /* sinusoidal data */
    /* all of these ** are accessed as [partial][frame] */
    double  **time;
    double  **frq;
    double  **amp;
    double  **pha;
    double  **smr;
    /* noise data */
    int32_t     *bands;
    double  **res;
    double  **band_energy;
} ATS_SOUND;

/* Interface:
 * ==========
 * grouped by file in alphabetical order
 */

/* atsa.c */

/* main_anal
 * =========
 * main analysis function
 * soundfile: path to input file
 * out_file: path to output ats file
 * anargs: pointer to analysis parameters
 * resfile: path to residual file
 * returns error status
 */
static int32_t main_anal(CSOUND *csound, char *soundfile, char *ats_outfile,
                     ANARGS *anargs, char *resfile);

/* critical-bands.c */

/* evaluate_smr
 * ============
 * evalues the masking curves of an analysis frame
 * peaks: pointer to an array of peaks
 * peaks_size: number of peaks
 */
static void evaluate_smr(ATS_PEAK *peaks, int32_t peaks_size);

/* other-utils.c */

/* window_norm
 * ===========
 * computes the norm of a window
 * returns the norm value
 * win: pointer to a window
 * size: window size
 */
static float window_norm(float *win, int32_t size);

/* make_window
 * ===========
 * makes an analysis window, returns a pointer to it.
 * win_type: window type, available types are:
 * BLACKMAN, BLACKMAN_H, HAMMING and VONHANN
 * win_size: window size
 */
static float *make_window(CSOUND *csound, int32_t win_type, int32_t win_size);

/* push_peak
 * =========
 * pushes a peak into an array of peaks
 * re-allocating memory and updating its size
 * returns a pointer to the array of peaks.
 * new_peak: pointer to new peak to push into the array
 * peaks_list: list of peaks
 * peaks_size: pointer to the current size of the array.
 */
static ATS_PEAK *push_peak(CSOUND *csound, ATS_PEAK *new_peak,
                           ATS_PEAK *peaks, int32_t *peaks_size);

/* peak_frq_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing frequency order.
 */
static int32_t peak_frq_inc(void const *a, void const *b);

/* peak_amp_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing amplitude order.
 */
static int32_t peak_amp_inc(void const *a, void const *b);

#if 0
/* peak_smr_dec
 * ============
 * function used by qsort to sort an array of peaks
 * in decreasing SMR order.
 */
static int32_t peak_smr_dec(void const *a, void const *b);
#endif

/* peak-detection.c */

/* peak_detection
 * ==============
 * detects peaks in a ATS_FFT block
 * returns an array of detected peaks.
 * ats_fft: pointer to ATS_FFT structure
 * lowest_bin: lowest fft bin to start detection
 * highest_bin: highest fft bin to end detection
 * lowest_mag: lowest magnitude to detect peaks
 * norm: analysis window norm
 * peaks_size: pointer to size of the returned peaks array
 */
static ATS_PEAK *peak_detection(CSOUND *csound, ATS_FFT *ats_fft,
                                int32_t lowest_bin, int32_t highest_bin,
                                float lowest_mag, double norm,
                                int32_t *peaks_size);

/* peak-tracking.c */

/* peak_tracking
 * =============
 * connects peaks from one analysis frame to tracks
 * returns a pointer to the analysis frame.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * peaks: peaks to connect
 * peaks_size: number of peaks
 * frq_dev: frequency deviation from tracks
 * SMR_cont: contribution of SMR to tracking
 * n_partials: pointer to the number of partials before tracking
 */
static ATS_FRAME *peak_tracking(CSOUND *csound, ATS_PEAK *tracks,
                                int32_t *tracks_size, ATS_PEAK *peaks,
                                int32_t *peaks_size, float frq_dev,
                                float SMR_cont, int32_t *n_partials);

/* update_tracks
 * =============
 * updates analysis tracks
 * returns a pointer to the tracks.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * track_len: length of tracks
 * frame_n: analysis frame number
 * ana_frames: pointer to previous analysis frames
 * last_peak_cont: contribution of last peak to the track
 */
static ATS_PEAK *update_tracks(CSOUND *csound, ATS_PEAK *tracks,
                               int32_t *tracks_size, int32_t track_len, int32_t frame_n,
                               ATS_FRAME *ana_frames, float last_peak_cont);

/* save-load-sound.c */

/* ats_save
 * ========
 * saves an ATS_SOUND to disk.
 * sound: pointer to ATS_SOUND structure
 * outfile: pointer to output ats file
 * SMR_thres: partials with and avreage SMR
 * below this value are considered masked
 * and not written out to the ats file
 * type: file type
 * NOTE: sound MUST be optimised using optimize_sound
 * before calling this function
 */
static void ats_save(CSOUND *csound, ATS_SOUND *sound, FILE *outfile,
                     float SMR_thres, int32_t type);

/* tracker.c */

/* tracker
 * =======
 * partial tracking function
 * returns an ATS_SOUND with data issued from analysis
 * anargs: pointer to analysis parameters
 * soundfile: path to input file
 * resfile: path to residual file
 */
static ATS_SOUND *tracker(CSOUND *csound, ANARGS *anargs, char *soundfile,
                          char *resfile);

/* utilities.c */

/* ppp2
 * ====
 * returns the closest power of two
 * greater than num
 */
static inline uint32_t ppp2(int32_t num);

/* various conversion functions
 * to deal with dB and dB SPL
 * they take and return double floats
 */
static inline double amp2db(double amp);
static inline double db2amp(double db);
static inline double amp2db_spl(double amp);
// static inline double db2amp_spl(double db_spl);

/* optimize_sound
 * ==============
 * optimises an ATS_SOUND in memory before saving
 * anargs: pointer to analysis parameters
 * sound: pointer to ATS_SOUND structure
 */
static void optimize_sound(CSOUND *csound, ANARGS *anargs, ATS_SOUND *sound);

/* residual.c */

/* compute_residual
 * ================
 * Computes the difference between the synthesis and the original sound.
 * the <win-samps> array contains the sample numbers in the input file
 * corresponding to each frame
 * fil: pointer to analysed data
 * fil_len: length of data in samples
 * output_file: output file path
 * sound: pointer to ATS_SOUND
 * win_samps: pointer to array of analysis windows center times
 * file_sampling_rate: sampling rate of analysis file
 */
static void compute_residual(CSOUND *csound, mus_sample_t **fil,
                             int32_t fil_len, char *output_file,
                             ATS_SOUND *sound, int32_t *win_samps,
                             int32_t file_sampling_rate);

/* residual-analysis.c */

/* residual_analysis
 * =================
 * performs the critical-band analysis of the residual file
 * file: name of the sound file containing the residual
 * sound: sound to store the residual data
 */
static void residual_analysis(CSOUND *csound, char *file, ATS_SOUND *sound);

#if 0
/* band_energy_to_res
 * ==================
 * transfers residual engergy from bands to partials
 * sound: sound structure containing data
 * frame: frame number
 */
static void band_energy_to_res(CSOUND *csound, ATS_SOUND *sound, int32_t frame);
#endif

#if 0
/* res_to_band_energy
 * ==================
 * transfers residual engergy from partials to bands
 * sound: sound structure containing data
 * frame: frame number
 */
static void res_to_band_energy(ATS_SOUND *sound, int32_t frame);
#endif

/* init_sound
 * ==========
 * initialises a new sound allocating memory
 */
static void init_sound(CSOUND *csound, ATS_SOUND *sound, int32_t sampling_rate,
                       int32_t frame_size, int32_t window_size, int32_t frames,
                       double duration, int32_t partials, int32_t use_noise);

/* free_sound
 * ==========
 * frees sound's memory
 */
static void free_sound(CSOUND *csound, ATS_SOUND *sound);

 /* ------------------------------------------------------------------------ */

/* main_anal
 * =========
 * main analysis function
 * soundfile: path to input file
 * out_file: path to output ats file
 * anargs: pointer to analysis parameters
 * returns error status
 */
static int32_t main_anal(CSOUND *csound, char *soundfile, char *ats_outfile,
                     ANARGS *anargs, char *resfile)
{
    /* create pointers and structures */
    ATS_SOUND *sound = NULL;
    FILE    *outfile;
    void    *fd;

    /* open output file */
    fd = csound->FileOpen(csound, &outfile, CSFILE_STD, ats_outfile, "wb",
                          NULL, CSFTYPE_ATS, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->Die(csound, Str("\nCould not open %s for writing, %s\nbye...\n"),
                  ats_outfile, Str(csound->SndfileStrError(csound,NULL)));
    }
    /* call tracker */
    sound = tracker(csound, anargs, soundfile, resfile);
    /* save sound */
    if (LIKELY(sound != NULL)) {
      csound->Message(csound, "%s", Str("saving ATS data..."));
      ats_save(csound, sound, outfile, anargs->SMR_thres, anargs->type);
      csound->Message(csound, "%s", Str("done!\n"));
    }
    else {
      /* file I/O error */
      return -2;
    }
    /* close output file */
    csound->FileClose(csound, fd);
    /* free ATS_SOUND memory */
    free_sound(csound, sound);
    return 0;
}

 /* ------------------------------------------------------------------------ */

static CS_NOINLINE CS_NORETURN void usage(CSOUND *csound)
{
    csound->Message(csound, "ATSA 1.0\n");
    csound->Message(csound, "%s", Str("atsa soundfile atsfile [flags]\n"));
    csound->Message(csound, "%s", Str("Flags:\n"));
    csound->Message(csound, Str("\t -b start (%f seconds)\n"), ATSA_START);
    csound->Message(csound, Str("\t -e duration (%f seconds or end)\n"),
                    ATSA_DUR);
    csound->Message(csound, Str("\t -l lowest frequency (%f Hertz)\n"),
                    ATSA_LFREQ);
    csound->Message(csound, Str("\t -H highest frequency (%f Hertz)\n"),
                    ATSA_HFREQ);
    csound->Message(csound,
                    Str("\t -d frequency deviation (%f of partial freq.)\n"),
                    ATSA_FREQDEV);
    csound->Message(csound, Str("\t -c window cycles (%d cycles)\n"),
                    ATSA_WCYCLES);
    csound->Message(csound, Str("\t -w window type (type: %d)\n"), ATSA_WTYPE);
    csound->Message(csound, "%s", Str("\t\t(Options: 0=BLACKMAN, 1=BLACKMAN_H, "
                                "2=HAMMING, 3=VONHANN)\n"));
    csound->Message(csound, Str("\t -h hop size (%f of window size)\n"),
                    ATSA_HSIZE);
    csound->Message(csound, Str("\t -m lowest magnitude (%f)\n"), ATSA_LMAG);
    csound->Message(csound, Str("\t -t track length (%d frames)\n"),
                    ATSA_TRKLEN);
    csound->Message(csound, Str("\t -s min. segment length (%d frames)\n"),
                    ATSA_MSEGLEN);
    csound->Message(csound, Str("\t -g min. gap length (%d frames)\n"),
                    ATSA_MGAPLEN);
    csound->Message(csound, Str("\t -T SMR threshold (%f dB SPL)\n"),
                    ATSA_SMRTHRES);
    csound->Message(csound, Str("\t -S min. segment SMR (%f dB SPL)\n"),
                    ATSA_MSEGSMR);
    csound->Message(csound,  Str("\t -P last peak contribution "
                                "(%f of last peak's parameters)\n"),
                    ATSA_LPKCONT);
    csound->Message(csound, Str("\t -M SMR contribution (%f)\n"), ATSA_SMRCONT);
    csound->Message(csound, Str("\t -F File Type (type: %d)\n"), ATSA_TYPE);
    csound->Message(csound, "%s", Str("\t\t(Options: 1=amp.and freq. only, "
                                "2=amp.,freq. and phase, "
                                "3=amp.,freq. and residual, "
                                "4=amp.,freq.,phase, and residual)\n\n"));
    csound->LongJmp(csound, 1);
}

static int32_t atsa_main(CSOUND *csound, int32_t argc, char **argv)
{
    int32_t     i, val, end_of_flags = 0;
    ANARGS  *anargs;
    char    *soundfile = (char *) NULL, *ats_outfile = (char *) NULL;
    char    *s = (char *) NULL;
    char    cur_opt = '\0';

    anargs = (ANARGS *) csound->Calloc(csound, sizeof(ANARGS));

    /* default values for analysis args */
    anargs->start = ATSA_START;
    anargs->duration = ATSA_DUR;
    anargs->lowest_freq = ATSA_LFREQ;
    anargs->highest_freq = ATSA_HFREQ;
    anargs->freq_dev = ATSA_FREQDEV;
    anargs->win_cycles = ATSA_WCYCLES;
    anargs->win_type = ATSA_WTYPE;
    anargs->hop_size = ATSA_HSIZE;
    anargs->lowest_mag = ATSA_LMAG;
    anargs->track_len = ATSA_TRKLEN;
    anargs->min_seg_len = ATSA_MSEGLEN;
    anargs->min_gap_len = ATSA_MGAPLEN;
    anargs->SMR_thres = ATSA_SMRTHRES;
    anargs->min_seg_SMR = ATSA_MSEGSMR;
    anargs->last_peak_cont = ATSA_LPKCONT;
    anargs->SMR_cont = ATSA_SMRCONT;
    anargs->type = ATSA_TYPE;

    for (i = 1; i < argc; ++i) {
      if (cur_opt == '\0') {
        if (argv[i][0] != '-' || end_of_flags) {
          if (soundfile == NULL)
            soundfile = argv[i];
          else if (ats_outfile == NULL)
            ats_outfile = argv[i];
          else
            usage(csound);
          continue;
        }
        else if (argv[i][1] == '-' && argv[i][2] == '\0') {
          end_of_flags = 1;
          continue;
        }
        else if (argv[i][1] == '\0')
          usage(csound);
        else {
          cur_opt = argv[i][1];
          s = &(argv[i][2]);
          if (*s == '\0')
            continue;
        }
      }
      else
        s = argv[i];
      if (*s == '\0')
        usage(csound);
      switch (cur_opt) {
      case 'b':
        anargs->start = (float) atof(s);
        break;
      case 'e':
        anargs->duration = (float) atof(s);
        break;
      case 'l':
        anargs->lowest_freq = (float) atof(s);
        break;
      case 'H':
        anargs->highest_freq = (float) atof(s);
        break;
      case 'd':
        anargs->freq_dev = (float) atof(s);
        break;
      case 'c':
        anargs->win_cycles = (int) atoi(s);
        break;
      case 'w':
        anargs->win_type = (int) atoi(s);
        break;
      case 'h':
        anargs->hop_size = (float) atof(s);
        break;
      case 'm':
        anargs->lowest_mag = (float) atof(s);
        break;
      case 't':
        anargs->track_len = (int) atoi(s);
        break;
      case 's':
        anargs->min_seg_len = (int) atoi(s);
        break;
      case 'g':
        anargs->min_gap_len = (int) atoi(s);
        break;
      case 'T':
        anargs->SMR_thres = (float) atof(s);
        break;
      case 'S':
        anargs->min_seg_SMR = (float) atof(s);
        break;
      case 'P':
        anargs->last_peak_cont = (float) atof(s);
        break;
      case 'M':
        anargs->SMR_cont = (float) atof(s);
        break;
      case 'F':
        anargs->type = (int) atoi(s);
        break;
      default:
        usage(csound);
      }
      cur_opt = '\0';
    }
    if (cur_opt != '\0' ||
        soundfile == NULL || soundfile[0] == '\0' ||
        ats_outfile == NULL || ats_outfile[0] == '\0')
      usage(csound);
#ifdef WIN32
    {
      char buffer[160];
      char * tmp = getenv("TEMP");
      strNcpy(buffer, tmp, 160);
      // MKG 2014 Jan 29: No linkage for strlcat with MinGW here.
      // but wrong; corrected
      //strlcat(buffer, ATSA_RES_FILE, 160);
      strncat(buffer, ATSA_RES_FILE, 160-strlen(buffer)); buffer[159] = '\0';
      val = main_anal(csound, soundfile, ats_outfile, anargs, buffer);
    }
#else
    val = main_anal(csound, soundfile, ats_outfile, anargs, ATSA_RES_FILE);
#endif
    csound->Free(csound, anargs);
    return (val);
}

 /* ------------------------------------------------------------------------ */

/* private function prototypes */
static void clear_mask(ATS_PEAK *peaks, int32_t peaks_size);
static double compute_slope_r(double val);
static double frq2bark(double frq, double *edges);
static int32_t find_band(double frq, double *edges);

/* frq2bark
 * ========
 * frequency to bark scale conversion
 */
static double frq2bark(double frq, double *edges)
{
    double  lo_frq, hi_frq;
    int32_t     band;

    if (frq <= 400.0)
      return (frq * 0.01);
    if (UNLIKELY(frq >= 20000.0))
      return (NIL);

    band = find_band(frq, edges);
    lo_frq = edges[band];
    hi_frq = edges[band + 1];
    return (1.0 + band + fabs(log10(frq / lo_frq) / log10(lo_frq / hi_frq)));
}

/* find_band
 * =========
 * returns the critical band number
 * corresponding to frq
 */
static int32_t find_band(double frq, double *edges)
{
    int32_t     i = 0;

    while (frq > edges[i++]);
    return (i - 2);
}

/* compute_slope_r
 * ===============
 * computes masking curve's right slope from val
 */
static double compute_slope_r(double val)
{
    double  i = val - 40.0;

    return (((i > 0.0) ? i : 0.0) * 0.37 - 27.0);
}

/* clear_mask
 * ==========
 * clears masking curves
 * peaks: array of peaks representing the masking curve
 * peaks_size: number of peaks in curve
 */
static void clear_mask(ATS_PEAK *peaks, int32_t peaks_size)
{
    while (peaks_size--)
      peaks[peaks_size].smr = 0.0;
}

/* evaluate_smr
 * ============
 * evalues the masking curves of an analysis frame
 * setting the peaks smr slot.
 * peaks: pointer to an array of peaks
 * peaks_size: number of peaks
 */
static void evaluate_smr(ATS_PEAK *peaks, int32_t peaks_size)
{
    double  slope_l = -27.0, slope_r, delta_dB = -50.0;
    double  frq_masker, amp_masker, frq_maskee, amp_maskee, mask_term;
    int32_t     i, j;
    ATS_PEAK *maskee;
    double  edges[ATSA_CRITICAL_BANDS + 1] = ATSA_CRITICAL_BAND_EDGES;

    clear_mask(peaks, peaks_size);
    if (peaks_size == 1)
      peaks[0].smr = amp2db_spl(peaks[0].amp);
    else
      for (i = 0; i < peaks_size; i++) {
        maskee = &peaks[i];
        frq_maskee = frq2bark(maskee->frq, edges);
        amp_maskee = amp2db_spl(maskee->amp);
        for (j = 0; j < peaks_size; j++)
          if (i != j) {
            frq_masker = frq2bark(peaks[j].frq, edges);
            amp_masker = amp2db_spl(peaks[j].amp);
            slope_r = compute_slope_r(amp_masker);
            mask_term = (frq_masker < frq_maskee) ?
                (amp_masker + delta_dB +
                 (slope_r * (frq_maskee - frq_masker))) : (amp_masker +
                                                           delta_dB +
                                                           (slope_l *
                                                            (frq_masker -
                                                             frq_maskee)));
            if (mask_term > maskee->smr)
              maskee->smr = mask_term;
          }
        maskee->smr = amp_maskee - maskee->smr;
      }
}

 /* ------------------------------------------------------------------------ */

/* make_window
 * ===========
 * makes an analysis window, returns a pointer to it.
 * win_type: window type, available types are:
 * BLACKMAN, BLACKMAN_H, HAMMING and VONHANN
 * win_size: window size
 */
static float *make_window(CSOUND *csound, int32_t win_type, int32_t win_size)
{
    float   *buffer;
    int32_t     i;
    float   arg = TWOPI / (float) (win_size - 1);

    buffer = (float *) csound->Malloc(csound, win_size * sizeof(float));

    for (i = 0; i < win_size; i++) {
      switch (win_type) {
      case BLACKMAN:           /* Blackman (3 term) */
        buffer[i] = (float)(0.42 - 0.5 * cos(arg * i) + 0.08 * cos(arg * (i+i)));
        break;
      case BLACKMAN_H:         /* Blackman-Harris (4 term) */
        buffer[i] =(float)(
            0.35875 - 0.48829 * cos(arg * i) + 0.14128 * cos(arg * (i+i)) -
            0.01168 * cos(arg * (i+i+i)));
        break;
      case HAMMING:           /* Hamming */
        buffer[i] = (float)(0.54 - 0.46 * cos(arg * i));
        break;
      case VONHANN:           /* Von Hann ("hanning") */
        buffer[i] = (float)(0.5 - 0.5 * cos(arg * i));
        break;
      }
    }
    return (buffer);
}

/* window_norm
 * ===========
 * computes the norm of a window
 * returns the norm value
 * win: pointer to a window
 * size: window size
 */
static float window_norm(float *win, int32_t size)
{
    float   acc = 0.0f;
    int32_t     i;

    for (i = 0; i < size; i++) {
      acc += win[i];
    }
    return (2.0f / acc);
}

/* push_peak
 * =========
 * pushes a peak into an array of peaks
 * re-allocating memory and updating its size
 * returns a pointer to the array of peaks.
 * new_peak: pointer to new peak to push into the array
 * peaks_list: list of peaks
 * peaks_size: pointer to the current size of the array.
 */
static ATS_PEAK *push_peak(CSOUND *csound, ATS_PEAK *new_peak,
                           ATS_PEAK *peaks_list, int32_t *peaks_size)
{
    peaks_list =
        (ATS_PEAK *) csound->ReAlloc(csound, peaks_list,
                                     sizeof(ATS_PEAK) * ++*peaks_size);
    peaks_list[*peaks_size - 1] = *new_peak;
    return (peaks_list);
}

/* peak_frq_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing frequency order.
 */
static int32_t peak_frq_inc(void const *a, void const *b)
{
    return (int)(1000.0 * (((ATS_PEAK *) a)->frq - ((ATS_PEAK *) b)->frq));
}

/* peak_amp_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing amplitude order.
 */
static int32_t peak_amp_inc(void const *a, void const *b)
{
    return (int)(1000.0 * (((ATS_PEAK *) a)->amp - ((ATS_PEAK *) b)->amp));
}

#if 0
/* peak_smr_dec
 * ============
 * function used by qsort to sort an array of peaks
 * in decreasing SMR order.
 */
static int32_t peak_smr_dec(void const *a, void const *b)
{
    return (int)(1000.0 * (((ATS_PEAK *) b)->smr - ((ATS_PEAK *) a)->smr));
}
#endif

static CS_NOINLINE void atsa_sound_read_noninterleaved(CSOUND *csound, SNDFILE *sf,
                                                       mus_sample_t **bufs,
                                                       int32_t nChannels,
                                                       int32_t nFrames)
{
    mus_sample_t tmpBuf[128];
    int32_t     i, j, k, m, n;

    m = 128 / nChannels;
    k = m * nChannels;         /* samples in tmpBuf[] */
    j = k;                     /* position in tmpBuf[] */
    for (i = 0; i < nFrames; i++) {
      if (j >= k) {
        if ((nFrames - i) < m) {
          m = (nFrames - i);
          k = m * nChannels;
        }
        if (sizeof(mus_sample_t) == sizeof(float))
          n = (int) csound->SndfileRead(csound, sf, (void *) &(tmpBuf[0]), (sf_count_t) m);
        if (n < 0)
          n = 0;
        n *= nChannels;
        for (; n < k; n++)
          tmpBuf[n] = (mus_sample_t) 0;
        j = 0;
      }
      for (n = 0; n < nChannels; n++)
        bufs[n][i] = tmpBuf[j++];
    }
}

static CS_NOINLINE void atsa_sound_write_noninterleaved(CSOUND *csound, SNDFILE *sf,
                                                        mus_sample_t **bufs,
                                                        int32_t nChannels,
                                                        int32_t nFrames)
{
    mus_sample_t tmpBuf[128];
    int32_t     i, j, k, m, n;

    m = 128 / nChannels;
    k = m * nChannels;         /* samples in tmpBuf[] */
    j = 0;                     /* position in tmpBuf[] */
    for (i = 0; i < nFrames; i++) {
      for (n = 0; n < nChannels; n++)
        tmpBuf[j++] = bufs[n][i];
      if (j >= k || i == (nFrames - 1)) {
        n = j / nChannels;
        if (sizeof(mus_sample_t) == sizeof(float))
          n = (int) csound->SndfileWrite(csound, sf, (void *) &(tmpBuf[0]), (sf_count_t) m);
        j = 0;
      }
    }
}

 /* ------------------------------------------------------------------------ */

/* private function prototypes */
static void parabolic_interp(double alpha, double beta, double gamma,
                             double *offset, double *height);
static double phase_interp(double PeakPhase, double OtherPhase, double offset);
static void to_polar(ATS_FFT *ats_fft, double *mags, double *phase, int32_t N,
                     double norm);

/* peak_detection
 * ==============
 * detects peaks in a ATS_FFT block
 * returns pointer to an array of detected peaks.
 * ats_fft: pointer to ATS_FFT structure
 * lowest_bin: lowest fft bin to start detection
 * highest_bin: highest fft bin to end detection
 * lowest_mag: lowest magnitude to detect peaks
 * norm: analysis window norm
 * peaks_size: pointer to size of the returned peaks array
 */
static ATS_PEAK *peak_detection(CSOUND *csound, ATS_FFT *ats_fft,
                                int32_t lowest_bin, int32_t highest_bin,
                                float lowest_mag, double norm,
                                int32_t *peaks_size)
{
    int32_t     k, N = (highest_bin ? highest_bin : ats_fft->size / 2);
    int32_t     first_bin = (lowest_bin ? ((lowest_bin > 2) ? lowest_bin : 2) : 2);
    double  fft_mag =
        ((double) ats_fft->rate / ats_fft->size), *fftmags, *fftphase;
    double  right_bin, left_bin, central_bin, offset;
    ATS_PEAK ats_peak, *peaks = NULL;

    lowest_mag = (float) db2amp(lowest_mag);

    /* init peak */
    ats_peak.amp = 0.0;
    ats_peak.frq = 0.0;
    ats_peak.pha = 0.0;
    ats_peak.smr = 0.0;

    fftmags = (double *) csound->Malloc(csound, N * sizeof(double));
    fftphase = (double *) csound->Malloc(csound, N * sizeof(double));
    /* convert spectrum to polar coordinates */
    to_polar(ats_fft, fftmags, fftphase, N, norm);
    central_bin = fftmags[first_bin - 2];
    right_bin = fftmags[first_bin - 1];
    /* peak detection */
    for (k = first_bin; k < N; k++) {
      left_bin = central_bin;
      central_bin = right_bin;
      right_bin = fftmags[k];
      if ((central_bin > (double) lowest_mag) && (central_bin > right_bin) &&
          (central_bin > left_bin)) {
        parabolic_interp(left_bin, central_bin, right_bin, &offset,
                         &ats_peak.amp);
        ats_peak.frq = fft_mag * ((k - 1) + offset);
        ats_peak.pha =
            (offset < 0.0) ? phase_interp(fftphase[k - 2], fftphase[k - 1],
                                          fabs(offset))
                             : phase_interp(fftphase[k - 1], fftphase[k],
                                            offset);
        ats_peak.track = -1;
        /* push peak into peaks list */
        peaks = push_peak(csound, &ats_peak, peaks, peaks_size);
      }
    }
    /* free up fftmags and fftphase */
    csound->Free(csound, fftmags);
    csound->Free(csound, fftphase);
    return (peaks);
}

/* to_polar
 * ========
 * rectangular to polar conversion
 * values are also scaled by window norm
 * and stored into separate arrays of
 * magnitudes and phases.
 * ats_fft: pointer to ATS_FFT structure
 * mags: pointer to array of magnitudes
 * phase: pointer to array of phases
 * N: highest bin in fft data array
 * norm: window norm used to scale magnitudes
 */
static void to_polar(ATS_FFT *ats_fft, double *mags, double *phase, int32_t N,
                     double norm)
{
    int32_t     k;
    double  x, y;

    for (k = 0; k < N; k++) {
      x = (double) ats_fft->data[k << 1];
      y = (double) ats_fft->data[(k << 1) + 1];
      mags[k] = norm * hypot(x, y);
      phase[k] = ((x == 0.0 && y == 0.0) ? 0.0 : atan2(y, x));
    }
}

/* parabolic_interp
 * ================
 * parabolic peak interpolation
 */
static void parabolic_interp(double alpha, double beta, double gamma,
                             double *offset, double *height)
{
    double  dbAlpha = amp2db(alpha), dbBeta = amp2db(beta), dbGamma = amp2db(gamma);
    *offset = 0.5 * ((dbAlpha - dbGamma) / (dbAlpha - 2.0 * dbBeta + dbGamma));
    *height = db2amp(dbBeta - ((dbAlpha - dbGamma) * 0.25 * *offset));
}

/* phase_interp
 * ============
 * phase interpolation
 */
static double phase_interp(double PeakPhase, double RightPhase, double offset)
{
    if ((PeakPhase - RightPhase) > PI * 1.5)
      RightPhase += TWOPI;
    else if ((RightPhase - PeakPhase) > PI * 1.5)
      PeakPhase += TWOPI;
    return ((RightPhase - PeakPhase) * offset + PeakPhase);
}

 /* ------------------------------------------------------------------------ */

/* private types */
typedef struct {
    int32_t     size;
    ATS_PEAK *cands;
} ATS_CANDS;

/* private function prototypes */
static ATS_PEAK *find_candidates(CSOUND *csound, ATS_PEAK *peaks,
                                 int32_t peaks_size, double lo, double hi,
                                 int32_t *cand_size);
static void sort_candidates(ATS_CANDS *cands, ATS_PEAK peak, float SMR_cont);

/* peak_tracking
 * =============
 * connects peaks from one analysis frame to tracks
 * returns a pointer to two frames of orphaned peaks.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * peaks: peaks to connect
 * peaks_size: number of peaks
 * frq_dev: frequency deviation from tracks
 * SMR_cont: contribution of SMR to tracking
 * n_partials: pointer to the number of partials before tracking
 */
static ATS_FRAME *peak_tracking(CSOUND *csound, ATS_PEAK *tracks,
                                int32_t *tracks_size, ATS_PEAK *peaks,
                                int32_t *peaks_size, float frq_dev,
                                float SMR_cont, int32_t *n_partials)
{
    ATS_CANDS *track_candidates =
        (ATS_CANDS *) csound->Malloc(csound, *peaks_size * sizeof(ATS_CANDS));
    double  lo, hi;
    int32_t     k, j, used, goback;
    ATS_FRAME *returned_peaks =
        (ATS_FRAME *) csound->Malloc(csound, 2 * sizeof(ATS_FRAME));

    returned_peaks[0].peaks = returned_peaks[1].peaks = NULL;
    returned_peaks[0].n_peaks = returned_peaks[1].n_peaks = 0;

    /* sort data to prepare for matching */
    qsort(tracks, *tracks_size, sizeof(ATS_PEAK), peak_frq_inc);
    qsort(peaks, *peaks_size, sizeof(ATS_PEAK), peak_frq_inc);

    /* find candidates for each peak and set each peak to best candidate */
    for (k = 0; k < *peaks_size; k++) {
      /* find frq limits for candidates */
      lo = peaks[k].frq - (0.5 * peaks[k].frq * frq_dev);
      hi = peaks[k].frq + (0.5 * peaks[k].frq * frq_dev);
      /* get possible candidates */
      track_candidates[k].size = 0;
      track_candidates[k].cands =
          find_candidates(csound, tracks, *tracks_size, lo, hi,
                          &track_candidates[k].size);
      if (track_candidates[k].size) {
        sort_candidates(&track_candidates[k], peaks[k], SMR_cont);
        peaks[k].track = track_candidates[k].cands[0].track;
      }
    }

    /* compare adjacent peaks track numbers to insure unique track numbers */
    do {
      goback = 0;
      for (j = 0; j < (*peaks_size - 1); j++)
        if ((peaks[j].track == peaks[j + 1].track) && (peaks[j].track > -1)) {
          if (track_candidates[j].cands[0].amp >
              track_candidates[j + 1].cands[0].amp) {
            track_candidates[j].cands[0].amp = ATSA_HFREQ;
            qsort(track_candidates[j].cands, track_candidates[j].size,
                  sizeof(ATS_PEAK), peak_amp_inc);
            if (track_candidates[j].cands[0].amp < ATSA_HFREQ) {
              peaks[j].track = track_candidates[j].cands[0].track;
              goback = 1;
            }
            else
              peaks[j].track = -1;
          }
          else {
            track_candidates[j + 1].cands[0].amp = ATSA_HFREQ;
            qsort(track_candidates[j + 1].cands, track_candidates[j + 1].size,
                  sizeof(ATS_PEAK), peak_amp_inc);
            if (track_candidates[j + 1].cands[0].amp < ATSA_HFREQ)
              peaks[j + 1].track = track_candidates[j + 1].cands[0].track;
            else
              peaks[j + 1].track = -1;
          }
        }
    } while (goback);

    /* by this point, all peaks will either have a unique track number, or -1
       now we need to take care of those left behind */
    for (k = 0; k < *peaks_size; k++)
      if (peaks[k].track == -1) {
        peaks[k].track = (*n_partials)++;
        returned_peaks[1].peaks =
            push_peak(csound, &peaks[k], returned_peaks[1].peaks,
                      &returned_peaks[1].n_peaks);
      }

    /* check for tracks that didnt get assigned */
    for (k = 0; k < *tracks_size; k++) {
      used = 0;
      for (j = 0; j < *peaks_size; j++)
        if (tracks[k].track == peaks[j].track) {
          used = 1;
          break;
        }
      if (!used)
        returned_peaks[0].peaks =
            push_peak(csound, &tracks[k], returned_peaks[0].peaks,
                      &returned_peaks[0].n_peaks);
    }

    for (k = 0; k < *peaks_size; k++)
      csound->Free(csound, track_candidates[k].cands);
    csound->Free(csound, track_candidates);
    return (returned_peaks);
}

/* find_candidates
 * ===============
 * find candidates to continue a track form an array of peaks
 * returns a pointer to an array of candidates
 * peaks: pointer to array of peaks
 * peaks_size: number of peaks
 * lo: lowest frequency to consider candidates
 * hi: highest frequency to consider candidates
 * cand_size: pointer to the number of candidates returned
 */
static ATS_PEAK *find_candidates(CSOUND *csound, ATS_PEAK *peaks,
                                 int32_t peaks_size, double lo, double hi,
                                 int32_t *cand_size)
{
    int32_t     i;
    ATS_PEAK *cand_list = NULL;

    for (i = 0; i < peaks_size; i++)
      if ((lo <= peaks[i].frq) && (peaks[i].frq <= hi))
        cand_list = push_peak(csound, &peaks[i], cand_list, cand_size);

    return (cand_list);
}

/* sort_candidates
 * ===================
 * sorts candidates from best to worst according to frequency and SMR
 * peak_candidates: pointer to an array of candidate peaks
 * peak: the peak we are matching
 * SMR_cont: contribution of SMR to the matching
 */
static void sort_candidates(ATS_CANDS *cands, ATS_PEAK peak, float SMR_cont)
{
    int32_t     i;

    /* compute delta values and store them in cands.amp
       (dont worry, the candidate amps are useless otherwise!) */
    for (i = 0; i < cands->size; i++)
      cands->cands[i].amp =
          (fabs(cands->cands[i].frq - peak.frq) +
           (SMR_cont * fabs(cands->cands[i].smr - peak.smr))) / (SMR_cont + 1);

    /* sort list by amp (increasing) */
    qsort(cands->cands, cands->size, sizeof(ATS_PEAK), peak_amp_inc);
}

/* update_tracks
 * =============
 * updates analysis tracks
 * returns a pointer to the tracks.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * track_len: length of tracks
 * frame_n: analysis frame number
 * ana_frames: pointer to previous analysis frames
 * last_peak_cont: contribution of last peak to the track
 */
static ATS_PEAK *update_tracks(CSOUND *csound, ATS_PEAK *tracks,
                               int32_t *tracks_size, int32_t track_len, int32_t frame_n,
                               ATS_FRAME *ana_frames, float last_peak_cont)
{
    int32_t     frames, first_frame, track, g, i, k;
    double  frq_acc, last_frq, amp_acc, last_amp, smr_acc, last_smr;
    int32_t     f, a, s;
    ATS_PEAK *l_peaks, *peak;

    if (tracks != NULL) {
      frames = (frame_n < track_len) ? frame_n : track_len;
      first_frame = frame_n - frames;
      for (g = 0; g < *tracks_size; g++) {
        track = tracks[g].track;
        frq_acc = last_frq = amp_acc = last_amp = smr_acc = last_smr = 0.0;
        f = a = s = 0;
        for (i = first_frame; i < frame_n; i++) {
          l_peaks = ana_frames[i].peaks;
          peak = NULL;
          for (k = 0; k < ana_frames[i].n_peaks; k++)
            if (l_peaks[k].track == track) {
              peak = &l_peaks[k];
              break;
            }
          if (peak != NULL) {
            if (peak->frq > 0.0) {
              last_frq = peak->frq;
              frq_acc += peak->frq;
              f++;
            }
            if (peak->amp > 0.0) {
              last_amp = peak->amp;
              amp_acc += peak->amp;
              a++;
            }
            if (peak->smr > 0.0) {
              last_smr = peak->smr;
              smr_acc += peak->smr;
              s++;
            }
          }
        }
        if (f)
          tracks[g].frq =
              (last_peak_cont * last_frq) +
              ((1 - last_peak_cont) * (frq_acc / f));
        if (a)
          tracks[g].amp =
              (last_peak_cont * last_amp) +
              ((1 - last_peak_cont) * (amp_acc / a));
        if (s)
          tracks[g].smr =
              (last_peak_cont * last_smr) +
              ((1 - last_peak_cont) * (smr_acc / s));
      }
    }
    else
      for (g = 0; g < ana_frames[frame_n - 1].n_peaks; g++)
        tracks =
            push_peak(csound, &ana_frames[frame_n - 1].peaks[g], tracks,
                      tracks_size);

    return (tracks);
}

 /* ------------------------------------------------------------------------ */

#define ATSA_RES_MIN_FFT_SIZE 4096
#define ATSA_RES_PAD_FACTOR 2
#define MAG_SQUARED(re, im, norm) (norm * (re*re+im*im))

/* private function prototypes */
static int32_t residual_get_N(int32_t M, int32_t min_fft_size, int32_t factor);
static void residual_get_bands(double fft_mag, double *true_bands,
                               int32_t *limits, int32_t bands);
//static double residual_compute_time_domain_energy(ATS_FFT *fft_struct);
static double residual_get_band_energy(int32_t lo, int32_t hi, ATS_FFT *fft_struct,
                                       double norm);
static void residual_compute_band_energy(ATS_FFT *fft_struct,
                                         int32_t *band_limits, int32_t bands,
                                         double *band_energy, double norm);

static int32_t residual_get_N(int32_t M, int32_t min_fft_size, int32_t factor)
{
    int32_t     def_size = factor * M;

    while (def_size < min_fft_size)
      def_size = ppp2(def_size + 1);
    return (def_size);
}

static void residual_get_bands(double fft_mag, double *true_bands,
                               int32_t *limits, int32_t bands)
{
    int32_t     k;

    for (k = 0; k < bands; k++)
      limits[k] = (int)floor(true_bands[k] / fft_mag);
}

/* static double residual_compute_time_domain_energy(ATS_FFT *fft) */
/* { */
/*     /\* Parseval's Theorem states: */

/*        N-1                   N-1 */
/*        sum(|x(n)^2|) =  1/N* sum (|X(k)|^2) */
/*        n=0                   k=0 */

/*        then we multiply the time domain energy by 1/2 */
/*        because we only compute frequency energy between */
/*        0 Hz and Nyquist only (0 -> N/2) */
/*      *\/ */
/*     int32_t     n; */
/*     double  sum = 0.0; */

/*     for (n = 0; n < fft->size; n++) */
/*       sum += fabs((double) fft->data[n] * (double) fft->data[n]); */
/*     return (sum); */
/* } */

static double residual_get_band_energy(int32_t lo, int32_t hi, ATS_FFT *fft,
                                       double norm)
{
    /* does 1/N * sum(re^2+im^2) within a band around <center>
       from <lo> lower bin to <hi> upper bin in <fft-struct> */
    int32_t     k;
    double  sum = 0.0;

    if (lo < 0)
      lo = 0;
    if (hi > fft->size / 2)
      hi = fft->size / 2;       /* was (int)floor(fft->size * 0.5) */
    for (k = lo; k < hi; k++) {
      double  re = (double) fft->data[k << 1];
      double  im = (double) fft->data[(k << 1) + 1];

      sum += MAG_SQUARED(re, im, norm);
    }
    return (sum / (double) fft->size);
}

static void residual_compute_band_energy(ATS_FFT *fft, int32_t *band_limits,
                                         int32_t bands, double *band_energy,
                                         double norm)
{
    /* loop through bands and evaluate energy
       we compute energy of one band as:
       (N-1)/2
       1/N * sum(|X(k)|^2)
       k=0
       N=fft size, K=bins in band */
    int32_t     b;

    for (b = 0; b < bands - 1; b++)
      band_energy[b] =
          residual_get_band_energy(band_limits[b], band_limits[b + 1], fft,
                                   norm);
}

/* residual_analysis
 * =================
 * performs the critical-band analysis of the residual file
 * file: name of the sound file containing the residual
 * sound: sound to store the residual data
 */
static void residual_analysis(CSOUND *csound, char *file, ATS_SOUND *sound)
{
    int32_t     file_sampling_rate, sflen, hop, M, N, frames, *band_limits;
    int32_t     M_2, st_pt, filptr, i, frame_n, k;
    double  norm = 1.0, threshold, fft_mag, **band_arr = NULL, *band_energy;
    //double  time_domain_energy = 0.0, freq_domain_energy = 0.0, sum = 0.0;
    double  edges[ATSA_CRITICAL_BANDS + 1] = ATSA_CRITICAL_BAND_EDGES;
    ATS_FFT fft;
    SFLIB_INFO sfinfo;
    mus_sample_t **bufs;
    SNDFILE *sf;
    void    *fd, *setup;

    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    fd = csound->FileOpen(csound, &sf, CSFILE_SND_R, file, &sfinfo,  "SFDIR;SSDIR",
                           CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->Die(csound, Str("atsa: error opening residual file '%s'"), file);
    }
    if (UNLIKELY(sfinfo.channels != 2)) {
      csound->Die(csound,
                  Str("atsa: residual file has %d channels, must be stereo !"),
                  (int) sfinfo.channels);
    }
    file_sampling_rate = sfinfo.samplerate;
    sflen = (int) sfinfo.frames;
    hop = sound->frame_size;
    M = sound->window_size;
    N = residual_get_N(M, ATSA_RES_MIN_FFT_SIZE, ATSA_RES_PAD_FACTOR);
    setup = csound->RealFFTSetup(csound, N, FFT_FWD);
    bufs = (mus_sample_t **) csound->Malloc(csound, 2 * sizeof(mus_sample_t *));
    bufs[0] =
        (mus_sample_t *) csound->Malloc(csound, sflen * sizeof(mus_sample_t));
    bufs[1] =
        (mus_sample_t *) csound->Malloc(csound, sflen * sizeof(mus_sample_t));
    fft.size = N;
    fft.rate = file_sampling_rate;
    fft.data = (MYFLT *) csound->Malloc(csound, (N + 2) * sizeof(MYFLT));
    threshold = /*AMP_DB*/(ATSA_NOISE_THRESHOLD);
    frames = sound->frames;
    fft_mag = (double) file_sampling_rate / (double) N;
    band_limits =
        (int32_t *) csound->Malloc(csound, sizeof(int) * (ATSA_CRITICAL_BANDS + 1));
    residual_get_bands(fft_mag, edges, band_limits, ATSA_CRITICAL_BANDS + 1);
    band_arr = sound->band_energy;
    band_energy =
        (double *) csound->Malloc(csound, ATSA_CRITICAL_BANDS * sizeof(double));

    M_2 = (int)floor(((double) M - 1.0) * 0.5);
    st_pt = N - M_2;
    filptr = M_2 * -1;
    /* read sound into memory */
    atsa_sound_read_noninterleaved(csound, sf, bufs, 2, sflen);

    for (frame_n = 0; frame_n < frames; frame_n++) {
      for (i = 0; i < (N + 2); i++) {
        fft.data[i] = (MYFLT) 0;
      }
      for (k = 0; k < M; k++) {
        if (filptr >= 0 && filptr < sflen)
          fft.data[(k + st_pt) % N] = (MYFLT) bufs[0][filptr];
        filptr++;
      }
      //smp = filptr - M_2 - 1;
      //time_domain_energy = residual_compute_time_domain_energy(&fft);
      /* take the fft */
      csound->RealFFT(csound, setup, fft.data);
      residual_compute_band_energy(&fft, band_limits, ATSA_CRITICAL_BANDS + 1,
                                   band_energy, norm);
      //sum = 0.0;
      //for (k = 0; k < ATSA_CRITICAL_BANDS; k++) {
      //  sum += band_energy[k];
      //}
      //freq_domain_energy = 2.0 * sum;
      for (k = 0; k < ATSA_CRITICAL_BANDS; k++) {
        if (band_energy[k] < threshold) {
          band_arr[k][frame_n] = 0.0;
        }
        else {
          band_arr[k][frame_n] = band_energy[k];
        }
      }
      filptr = filptr - M + hop;
    }
    /* save data in sound */
    sound->band_energy = band_arr;
    csound->Free(csound, fft.data);
    csound->Free(csound, band_energy);
    csound->Free(csound, band_limits);
    csound->Free(csound, bufs[0]);
    csound->Free(csound, bufs[1]);
    csound->Free(csound, bufs);
}

#if 0
/* band_energy_to_res
 * ==================
 * transfers residual engergy from bands to partials
 * sound: sound structure containing data
 * frame: frame number
 */
static void band_energy_to_res(CSOUND *csound, ATS_SOUND *sound, int32_t frame)
{
    int32_t     i, j;
    double  edges[] = ATSA_CRITICAL_BAND_EDGES;
    double  bandsum[ATSA_CRITICAL_BANDS];
    double  partialfreq, partialamp;
    double  *partialbandamp;  /* amplitude of the band that the partial is in */
    int32_t     *bandnum;         /* the band number that the partial is in */

    partialbandamp = csound->Malloc(csound, sizeof(double) * sound->partials);
    bandnum = csound->Malloc(csound, sizeof(int) * sound->partials);
    /* initialise the sum per band */
    for (i = 0; i < ATSA_CRITICAL_BANDS; i++)
      bandsum[i] = 0;

    /* find find which band each partial is in */
    for (i = 0; i < sound->partials; i++) {
      partialfreq = sound->frq[i][frame];
      partialamp = sound->amp[i][frame];
      for (j = 0; j < 25; j++) {
        if ((partialfreq < edges[j + 1]) && (partialfreq >= edges[j])) {
          bandsum[j] += partialamp;
          bandnum[i] = j;
          partialbandamp[i] = sound->band_energy[j][frame];
          break;
        }
      }
    }
    /* compute energy per partial */
    for (i = 0; i < sound->partials; i++) {
      if (bandsum[bandnum[i]] > 0.0)
        sound->res[i][frame] =
            sound->amp[i][frame] * partialbandamp[i] / bandsum[bandnum[i]];
      else
        sound->res[i][frame] = 0.0;
    }
    csound->Free(csound, partialbandamp);
    csound->Free(csound, bandnum);
}
#endif

/* res_to_band_energy
 * ==================
 * transfers residual engergy from partials to bands
 * sound: sound structure containing data
 * frame: frame number
 */
#if 0
static void res_to_band_energy(ATS_SOUND *sound, int32_t frame)
{
    int32_t     j, par;
    double  sum;
    double  edges[ATSA_CRITICAL_BANDS + 1] = ATSA_CRITICAL_BAND_EDGES;

    par = 0;
    for (j = 0; j < ATSA_CRITICAL_BANDS; j++) {
      sum = 0.0;
      while (sound->frq[par][frame] >= edges[j] &&
             sound->frq[par][frame] < edges[j + 1]) {
        sum += sound->res[par][frame];
        par++;
      }
      sound->band_energy[j][frame] = sum;
    }
}
#endif

 /* ------------------------------------------------------------------------ */

/* private function prototypes */
static int32_t compute_m(double pha_1, double frq_1, double pha, double frq,
                     int32_t buffer_size);
static double compute_aux(double pha_1, double pha, double frq_1,
                          int32_t buffer_size, int32_t M);
static double compute_alpha(double aux, double frq_1, double frq,
                            int32_t buffer_size);
static double compute_beta(double aux, double frq_1, double frq,
                           int32_t buffer_size);
static double interp_phase(double pha_1, double frq_1, double alpha,
                           double beta, int32_t i);
static void read_frame(mus_sample_t **fil, int32_t fil_len, int32_t samp_1,
                       int32_t samp_2, double *in_buffer);
static void synth_buffer(double a1, double a2, double f1, double f2,
                         double p1, double p2, double *buffer,
                         int32_t frame_samps);

/* Functions for phase interpolation
 * All this comes from JOS/XJS article on PARSHL.
 * Original phase interpolation eqns. by Qualtieri/McAulay.
 */

static int32_t compute_m(double pha_1, double frq_1, double pha, double frq,
                     int32_t buffer_size)
{
 /* int32_t val = (int) ((((pha_1 + (frq_1 * (double) buffer_size) - pha)
                       + ((frq - frq_1) * 0.5 * (double) buffer_size)) / TWOPI)
                     + 0.5); */
    return ((int)
            ((((pha_1 + (frq_1 * (double) buffer_size) - pha) +
               ((frq - frq_1) * 0.5 * (double) buffer_size)) / TWOPI) + 0.5));
}

static double compute_aux(double pha_1, double pha, double frq_1,
                          int32_t buffer_size, int32_t M)
{
 /* double val = (double) ((pha + (TWOPI * (double) M))
                           - (pha_1 + (frq_1 * (double) buffer_size))); */
    return ((double)
            ((pha + (TWOPI * (double) M)) -
             (pha_1 + (frq_1 * (double) buffer_size))));
}

static double compute_alpha(double aux, double frq_1, double frq,
                            int32_t buffer_size)
{
 /* double val = (double) (((3.0 / (double) (buffer_size * buffer_size)) * aux)
                           - ((frq - frq_1) / (double) buffer_size)); */
    return ((double)
            (((3.0 / (double) (buffer_size * buffer_size)) * aux) -
             ((frq - frq_1) / (double) buffer_size)));
}

static double compute_beta(double aux, double frq_1, double frq,
                           int32_t buffer_size)
{
 /* double val = (double) (((-2.0 / (double) (buffer_size * buffer_size
                                              * buffer_size)) * aux)
                           + ((frq - frq_1)
                              / (double) (buffer_size * buffer_size))); */
    return ((double)
            (((-2.0 / (double) (buffer_size * buffer_size * buffer_size)) *
              aux) + ((frq - frq_1) / (double) (buffer_size * buffer_size))));
}

static double interp_phase(double pha_1, double frq_1, double alpha,
                           double beta, int32_t i)
{
 /* double val = (double) ((beta * (double) (i * i * i))
                           + (alpha * (double) (i * i))
                           + (frq_1 * (double) i) + pha_1); */
    return ((double)
            ((beta * (double) (i * i * i)) + (alpha * (double) (i * i)) +
             (frq_1 * (double) i) + pha_1));
}

/* read_frame
 * ==========
 * reads a frame from the input file
 * fil: pointer to an array with sound data
 * fil_len: length of datas in samples
 * samp_1: first sample number in frame
 * samp_2: last sample number in frame
 * in_buffer: pointer to input buffer
 * which is filled out by the function
 * NOTE: caller should allocate memory for buffer
 */
static void read_frame(mus_sample_t **fil, int32_t fil_len, int32_t samp_1,
                       int32_t samp_2, double *in_buffer)
{
    int32_t     i, index, samps = samp_2 - samp_1;
    mus_sample_t tmp;

    /* samps = samp_2 - samp_1; */
    for (i = 0; i < samps; i++) {
      index = samp_1 + i;
      if (index < fil_len)
        tmp = fil[0][index];
      else
        tmp = (mus_sample_t) 0.0;
      in_buffer[i] = (double) tmp;
    }
}

/* synth_buffer
 * ============
 * synthesizes a buffer of sound using
 * amplitude linear interpolation and
 * phase cubic interpolation
 * a1: strating amplitude
 * a2: ending amplitude
 * f1: starting frequency in radians per sample
 * f2: ending frequency in radians per sample
 * p1: starting phase in radians
 * p2: ending phase in radians
 * buffer: pointer to synthsis buffer
 * which is filled out by the function
 * NOTE: caller should allocate memory for buffer
 * frame_samps: number of samples in frame (buffer)
 */
static void synth_buffer(double a1, double a2, double f1, double f2,
                         double p1, double p2, double *buffer,
                         int32_t frame_samps)
{
    int32_t     k, M;
    double  aux, alpha, beta, amp, amp_inc, int_pha;

    M = compute_m(p1, f1, p2, f2, frame_samps);
    aux = compute_aux(p1, p2, f1, frame_samps, M);
    alpha = compute_alpha(aux, f1, f2, frame_samps);
    beta = compute_beta(aux, f1, f2, frame_samps);
    amp = a1;
    amp_inc = (a2 - a1) / (double) frame_samps;
    for (k = 0; k < frame_samps; k++) {
      int_pha = interp_phase(p1, f1, alpha, beta, k);
      buffer[k] += amp * cos(int_pha);
      amp += amp_inc;
    }
}

/* compute_residual
 * ================
 * Computes the difference between the synthesis and the original sound.
 * the <win-samps> array contains the sample numbers in the input file
 * corresponding to each frame
 * fil: pointer to analysed data
 * fil_len: length of data in samples
 * output_file: output file path
 * sound: pointer to ATS_SOUND
 * win_samps: pointer to array of analysis windows center times
 * file_sampling_rate: sampling rate of analysis file
 */
static void compute_residual(CSOUND *csound, mus_sample_t **fil,
                             int32_t fil_len, char *output_file,
                             ATS_SOUND *sound, int32_t *win_samps,
                             int32_t file_sampling_rate)
{
    int32_t     i, frm, frm_1, frm_2, par, frames, partials, frm_samps;
    double  *in_buff, *synth_buff, mag, a1, a2, f1, f2, p1, p2, diff, synth;
    mus_sample_t **obuf;
    SFLIB_INFO sfinfo;
    SNDFILE *sf;
    void    *fd;

    frames = sound->frames;
    partials = sound->partials;
    frm_samps = sound->frame_size;
    mag = TWOPI / (double) file_sampling_rate;
    in_buff = (double *) csound->Malloc(csound, frm_samps * sizeof(double));
    synth_buff = (double *) csound->Malloc(csound, frm_samps * sizeof(double));
    /* open output file */
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    //sfinfo.frames = (sflib_count_t)0; /* was -1 */
    sfinfo.samplerate = file_sampling_rate;
    sfinfo.channels = 2;
    sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
    fd = csound->FileOpen(csound, &sf, CSFILE_SND_W, output_file, &sfinfo,
                          NULL, CSFTYPE_WAVE, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->Die(csound, Str("\nERROR: cannot open file %s for writing\n"),
                  output_file);
    }
    csound->SndfileSetString(csound,sf, SF_STR_SOFTWARE, "created by ATSA");
    /* allocate memory */
    obuf = (mus_sample_t **) csound->Malloc(csound, 2 * sizeof(mus_sample_t *));
    obuf[0] =
        (mus_sample_t *) csound->Calloc(csound,
                                        frm_samps * sizeof(mus_sample_t));
    obuf[1] =
        (mus_sample_t *) csound->Calloc(csound,
                                        frm_samps * sizeof(mus_sample_t));
    /* compute residual frame by frame */
    for (frm = 1; frm < frames; frm++) {
      /* clean buffers up */
      memset(in_buff, '\0', frm_samps * sizeof(double));
      memset(synth_buff, '\0', frm_samps * sizeof(double));
      /* for (i = 0; i < frm_samps; i++) */
      /*   in_buff[i] = synth_buff[i] = 0.0; */
      frm_1 = frm - 1;
      frm_2 = frm;
      /* read frame from input */
      read_frame(fil, fil_len, win_samps[frm_1], win_samps[frm_2], in_buff);
      /* compute one synthesis frame */
      for (par = 0; par < partials; par++) {
        a1 = sound->amp[par][frm_1];
        a2 = sound->amp[par][frm_2];
        /*  have to convert the frequency into radians per sample!!! */
        f1 = sound->frq[par][frm_1] * mag;
        f2 = sound->frq[par][frm_2] * mag;
     /* f1 *= mag; */
     /* f2 *= mag; */
        p1 = sound->pha[par][frm_1];
        p2 = sound->pha[par][frm_2];
        if (!(a1 <= 0.0 && a2 <= 0.0)) {
          /* check amp 0 in frame 1 */
          if (a1 <= 0.0) {
            f1 = f2;
            p1 = p2 - (f2 * frm_samps);
            while (p1 > PI)
              p1 -= TWOPI;
            while (p1 < (PI * -1))
              p1 += TWOPI;
          }
          /* check amp 0 in frame 2 */
          if (a2 <= 0.0) {
            f2 = f1;
            p2 = p1 + (f1 * frm_samps);
            while (p2 > PI)
              p2 -= TWOPI;
            while (p2 < (PI * -1))
              p2 += TWOPI;
          }
          synth_buffer(a1, a2, f1, f2, p1, p2, synth_buff, frm_samps);
        }
      }
      /* write output: chan 0 residual chan 1 synthesis */
      for (i = 0; i < frm_samps; i++) {
        synth = synth_buff[i];
        diff = in_buff[i] - synth;
        obuf[0][i] = (mus_sample_t) diff;
        obuf[1][i] = (mus_sample_t) synth;
      }
      atsa_sound_write_noninterleaved(csound, sf, obuf, 2, frm_samps);
    }
    csound->Free(csound, in_buff);
    csound->Free(csound, synth_buff);
    /* update header and close output file */
    csound->FileClose(csound, fd);
    csound->Free(csound, obuf[0]);
    csound->Free(csound, obuf[1]);
    csound->Free(csound, obuf);
}

 /* ------------------------------------------------------------------------ */

/* ats_save
 * ========
 * saves an ATS_SOUND to disk.
 * sound: pointer to ATS_SOUND structure
 * outfile: pointer to output ats file
 * SMR_thres: partials with and avreage SMR
 * below this value are considered masked
 * and not written out to the ats file
 * type: file type
 * NOTE: sound MUST be optimised using optimize_sound
 * before calling this function
 */
static void ats_save(CSOUND *csound, ATS_SOUND *sound, FILE *outfile,
                     float SMR_thres, int32_t type)
{
    int32_t     frm, i, par, dead = 0;
    double  daux;
    ATS_HEADER header;

    if (UNLIKELY(sound->optimized == NIL)) {
      csound->Die(csound, "%s", Str("Error: sound not optimised !"));
    }
    /* count how many partials are dead
     * unfortunately we have to do this first to
     * write the number of partials in the header
     */
    for (i = 0; i < sound->partials; i++) {
      /* see if partial is dead */
      if (!(sound->av[i].frq > 0.0) || !(sound->av[i].smr >= SMR_thres)) {
        dead++;
      }
    }
    /* sort partials by increasing frequency */
    qsort(sound->av, sound->partials, sizeof(ATS_PEAK), peak_frq_inc);
    /* fill header up */
    header.mag = 123.0;
    header.sr  = (double) sound->srate;
    header.fs  = (double) sound->frame_size;
    header.ws  = (double) sound->window_size;
    header.par = (double) (sound->partials - dead);
    header.fra = (double) sound->frames;
    header.ma  = sound->ampmax;
    header.mf  = sound->frqmax;
    header.dur = sound->dur;
    header.typ = (double) type;
    /* write header */
    fseek(outfile, 0, SEEK_SET);
    if (UNLIKELY(1!=fwrite(&header, sizeof(ATS_HEADER), 1, outfile)))
      fprintf(stderr, "%s", Str("Write failure\n"));
    /* write frame data */
    for (frm = 0; frm < sound->frames; frm++) {
      daux = sound->time[0][frm];
      if (UNLIKELY(1!=fwrite(&daux, sizeof(double), 1, outfile)))
        fprintf(stderr, "%s", Str("Write failure\n"));
      for (i = 0; i < sound->partials; i++) {
        /* we ouput data in increasing frequency order
         * and we check for dead partials
         */
        if ((sound->av[i].frq > 0.0) && (sound->av[i].smr >= SMR_thres)) {
          /* get partial number from sound */
          par = sound->av[i].track;
          /* output data to file */
          daux = sound->amp[par][frm];
          if (UNLIKELY(1!=fwrite(&daux, sizeof(double), 1, outfile)))
            fprintf(stderr, "%s", Str("Write failure\n"));
          daux = sound->frq[par][frm];
          if (UNLIKELY(1!=fwrite(&daux, sizeof(double), 1, outfile)))
            fprintf(stderr, "%s", Str("Write failure\n"));
          if (type == 2 || type == 4) {
            daux = sound->pha[par][frm];
            if (UNLIKELY(1!=fwrite(&daux, sizeof(double), 1, outfile)))
              fprintf(stderr, "%s", Str("Write failure\n"));
          }
        }
      }
      /* write noise data */
      if (type == 3 || type == 4) {
        for (i = 0; i < ATSA_CRITICAL_BANDS; i++) {
          daux = sound->band_energy[i][frm];
          if (UNLIKELY(1!=fwrite(&daux, sizeof(double), 1, outfile)))
            fprintf(stderr, "%s", Str("Write failure\n"));
        }
      }
    }
}

 /* ------------------------------------------------------------------------ */

/* private function prototypes */
static int32_t compute_frames(ANARGS *anargs);

/* ATS_SOUND *tracker (ANARGS *anargs, char *soundfile)
 * partial tracking function
 * anargs: pointer to analysis parameters
 * soundfile: path to input file
 * returns an ATS_SOUND with data issued from analysis
 */
static ATS_SOUND *tracker(CSOUND *csound, ANARGS *anargs, char *soundfile,
                          char *resfile)
{
    int32_t     M_2, first_point, filptr, n_partials = 0;
    int32_t     frame_n, k, sflen, *win_samps, peaks_size, tracks_size = 0;
    int32_t     i, frame, i_tmp;
    float   *window, norm, sfdur, f_tmp;

    /* declare structures and buffers */
    ATS_SOUND *sound = NULL;
    ATS_PEAK *peaks, *tracks = NULL, cpy_peak;
    ATS_FRAME *ana_frames = NULL, *unmatched_peaks = NULL;
    mus_sample_t **bufs;
    ATS_FFT fft;
    SFLIB_INFO sfinfo;
    SNDFILE *sf;
    void    *fd , *setup;

    /* open input file
       we get srate and total_samps in file in anargs */
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    fd = csound->FileOpen(csound, &sf, CSFILE_SND_R, soundfile, &sfinfo,
                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->ErrorMsg(csound, Str("atsa: cannot open input file '%s': %s"),
                       soundfile, Str(csound->SndfileStrError(csound,NULL)));
      return NULL;
    }
    /* warn about multi-channel sound files */
    if (UNLIKELY(sfinfo.channels != 1)) {
      csound->ErrorMsg(csound,
                       Str("atsa: file has %d channels, must be mono !"),
                       (int) sfinfo.channels);
      return NULL;
    }

    csound->Message(csound, "%s", Str("tracking...\n"));

    /* get sample rate and # of frames from file header */
    anargs->srate = sfinfo.samplerate;
    sflen = (int) sfinfo.frames;
    sfdur = (float) sflen / anargs->srate;
    /* check analysis parameters */
    /* check start time */
    if (UNLIKELY(!(anargs->start >= 0.0 && anargs->start < sfdur))) {
      csound->Warning(csound, Str("start %f out of bounds, corrected to 0.0"),
                      anargs->start);
      anargs->start = 0.0f;
    }
    /* check duration */
    if (anargs->duration == ATSA_DUR) {
      anargs->duration = sfdur - anargs->start;
    }
    f_tmp = anargs->duration + anargs->start;
    if (UNLIKELY(!(anargs->duration > 0.0 && f_tmp <= sfdur))) {
      csound->Warning(csound, Str("duration %f out of bounds, "
                                  "limited to file duration"),
                      anargs->duration);
      anargs->duration = sfdur - anargs->start;
    }
    /* print time bounds */
    csound->Message(csound, Str("start: %f duration: %f file dur: %f\n"),
                    anargs->start, anargs->duration, sfdur);
    /* check lowest frequency */
    if (UNLIKELY(!
        (anargs->lowest_freq > 0.0 &&
         anargs->lowest_freq < anargs->highest_freq))) {
      csound->Warning(csound,
                      Str("lowest freq. %f out of bounds, "
                          "forced to default: %f"), anargs->lowest_freq,
                      ATSA_LFREQ);
      anargs->lowest_freq = ATSA_LFREQ;
    }
    /* check highest frequency */
    if (UNLIKELY(!
                 (anargs->highest_freq > anargs->lowest_freq &&
                  anargs->highest_freq <= anargs->srate * 0.5))) {
      csound->Warning(csound,
                      Str("highest freq. %f out of bounds, "
                          "forced to default: %f"), anargs->highest_freq,
                      ATSA_HFREQ);
      anargs->highest_freq = ATSA_HFREQ;
    }
    /* frequency deviation */
    if (UNLIKELY(!(anargs->freq_dev > 0.0f && anargs->freq_dev < 1.0f))) {
      csound->Warning(csound, Str("freq. dev. %f out of bounds, "
                                  "should be > 0.0 and <= 1.0, "
                                  "forced to default: %f"),
                      anargs->freq_dev, ATSA_FREQDEV);
      anargs->freq_dev = ATSA_FREQDEV;
    }
    /* window cycles */
    if (UNLIKELY(!(anargs->win_cycles >= 1 && anargs->win_cycles <= 8))) {
      csound->Warning(csound, Str("windows cycles %d out of bounds, "
                                  "should be between 1 and 8, "
                                  "forced to default: %d"),
                      anargs->win_cycles, ATSA_WCYCLES);
      anargs->win_cycles = ATSA_WCYCLES;
    }
    /* window type */
    if (UNLIKELY(!(anargs->win_type >= 0 && anargs->win_type <= 3))) {
      csound->Warning(csound, Str("window type %d out of bounds, "
                                  "should be between 0 and 3, "
                                  "forced to default: %d"),
                      anargs->win_type, ATSA_WTYPE);
      anargs->win_type = ATSA_WTYPE;
    }
    /* hop size */
    if (UNLIKELY(!(anargs->hop_size > 0.0 && anargs->hop_size <= 1.0))) {
      csound->Warning(csound, Str("hop size %f out of bounds, "
                                  "should be > 0.0 and <= 1.0, "
                                  "forced to default: %f"),
                      anargs->hop_size, ATSA_HSIZE);
      anargs->hop_size = ATSA_HSIZE;
    }
    /* lowest mag */
    if (UNLIKELY(!(anargs->lowest_mag <= 0.0))) {
      csound->Warning(csound, Str("lowest magnitude %f out of bounds, "
                                  "should be >= 0.0 and <= 1.0, "
                                  "forced to default: %f"),
                      anargs->lowest_mag, ATSA_LMAG);
      anargs->lowest_mag = ATSA_LMAG;
    }
    /* set some values before checking next set of parameters */
    anargs->first_smp = (int) floor(anargs->start * (float) anargs->srate);
    anargs->total_samps = (int) floor(anargs->duration * (float) anargs->srate);
    /* fundamental cycles */
    anargs->cycle_smp =
        (int) floor((double) anargs->win_cycles * (double) anargs->srate /
                    (double) anargs->lowest_freq);
    /* window size */
    anargs->win_size =
        (anargs->cycle_smp % 2 ==
         0) ? anargs->cycle_smp + 1 : anargs->cycle_smp;
    /* calculate hop samples */
    anargs->hop_smp = (int)floor((float) anargs->win_size * anargs->hop_size);
    /* compute total number of frames */
    anargs->frames = compute_frames(anargs);
    /* check that we have enough frames for the analysis */
    if (UNLIKELY(!(anargs->frames >= ATSA_MFRAMES))) {
      csound->ErrorMsg(csound,
                       Str("atsa: %d frames are not enough for analysis, "
                           "need at least %d"), anargs->frames, ATSA_MFRAMES);
      return NULL;
    }
    /* check other user parameters */
    /* track length */
    if (UNLIKELY(!(anargs->track_len >= 1 && anargs->track_len < anargs->frames))) {
      i_tmp = (ATSA_TRKLEN < anargs->frames) ? ATSA_TRKLEN : anargs->frames - 1;
      csound->Warning(csound,
                      Str("track length %d out of bounds, forced to: %d"),
                      anargs->track_len, i_tmp);
      anargs->track_len = i_tmp;
    }
    /* min. segment length */
    if (UNLIKELY(!(anargs->min_seg_len >= 1 &&
                   anargs->min_seg_len < anargs->frames))) {
      i_tmp =
          (ATSA_MSEGLEN < anargs->frames) ? ATSA_MSEGLEN : anargs->frames - 1;
      csound->Warning(csound,
                      Str("min. segment length %d out of bounds, "
                          "forced to: %d"), anargs->min_seg_len, i_tmp);
      anargs->min_seg_len = i_tmp;
    }
    /* min. gap length */
    if (UNLIKELY(!(anargs->min_gap_len >= 0 &&
                   anargs->min_gap_len < anargs->frames))) {
      i_tmp =
          (ATSA_MGAPLEN < anargs->frames) ? ATSA_MGAPLEN : anargs->frames - 1;
      csound->Warning(csound,
                      Str("min. gap length %d out of bounds, forced to: %d"),
                      anargs->min_gap_len, i_tmp);
      anargs->min_gap_len = i_tmp;
    }
    /* SMR threshold */
    if (UNLIKELY(!(anargs->SMR_thres >= 0.0 &&
                   anargs->SMR_thres < ATSA_MAX_DB_SPL))) {
      csound->Warning(csound, Str("SMR threshold %f out of bounds, "
                                  "should be >= 0.0 and < %f dB SPL, "
                                  "forced to default: %f"),
                      anargs->SMR_thres, ATSA_MAX_DB_SPL, ATSA_SMRTHRES);
      anargs->SMR_thres = ATSA_SMRTHRES;
    }
    /* min. seg. SMR */
    if (UNLIKELY(!
        (anargs->min_seg_SMR >= anargs->SMR_thres &&
         anargs->min_seg_SMR < ATSA_MAX_DB_SPL))) {
      csound->Warning(csound,
                      Str("min. seg. SMR %f out of bounds, "
                          "should be >= %f and < %f dB SPL, "
                          "forced to default: %f"), anargs->min_seg_SMR,
                      anargs->SMR_thres, ATSA_MAX_DB_SPL, ATSA_MSEGSMR);
      anargs->min_seg_SMR = ATSA_MSEGSMR;
    }
    /* last peak contribution */
    if (UNLIKELY(!(anargs->last_peak_cont >= 0.0 &&
                   anargs->last_peak_cont <= 1.0))) {
      csound->Warning(csound, Str("last peak contribution %f out of bounds, "
                                  "should be >= 0.0 and <= 1.0, "
                                  "forced to default: %f"),
                      anargs->last_peak_cont, ATSA_LPKCONT);
      anargs->last_peak_cont = ATSA_LPKCONT;
    }
    /* SMR cont. */
    if (UNLIKELY(!(anargs->SMR_cont >= 0.0 && anargs->SMR_cont <= 1.0))) {
      csound->Warning(csound, Str("SMR contribution %f out of bounds, "
                                  "should be >= 0.0 and <= 1.0, "
                                  "forced to default: %f"),
                      anargs->SMR_cont, ATSA_SMRCONT);
      anargs->SMR_cont = ATSA_SMRCONT;
    }
    /* continue computing parameters */
    /* fft size */
    anargs->fft_size = ppp2(2 * anargs->win_size);

    /* allocate memory for sound, we read the whole sound in memory */
    bufs = (mus_sample_t **) csound->Malloc(csound, sizeof(mus_sample_t *));
    bufs[0] =
        (mus_sample_t *) csound->Malloc(csound, sflen * sizeof(mus_sample_t));
    /* make our window */
    window = make_window(csound, anargs->win_type, anargs->win_size);
    /* get window norm */
    norm = window_norm(window, anargs->win_size);
    /* fft mag for computing frequencies */
    anargs->fft_mag = (double) anargs->srate / (double) anargs->fft_size;
    /* lowest fft bin for analysis */
    anargs->lowest_bin = (int)floor(anargs->lowest_freq / anargs->fft_mag);
    /* highest fft bin for analisis */
    anargs->highest_bin = (int)floor(anargs->highest_freq / anargs->fft_mag);
    /* allocate an array analysis frames in memory */
    ana_frames =
        (ATS_FRAME *) csound->Malloc(csound,
                                     anargs->frames * sizeof(ATS_FRAME));
    /* alocate memory to store mid-point window sample numbers */
    win_samps = (int32_t *) csound->Malloc(csound, anargs->frames * sizeof(int));
    /* center point of window */
    M_2 = (anargs->win_size-1)/2; /* Was (int)floor((anargs->win_size - 1) / 2) */
    /* first point in fft buffer to write */
    first_point = anargs->fft_size - M_2;
    /* half a window from first sample */
    filptr = anargs->first_smp - M_2;
    /* read sound into memory */
    atsa_sound_read_noninterleaved(csound, sf, bufs, 1, sflen);

    /* make our fft-struct */
    fft.size = anargs->fft_size;
    setup = csound->RealFFTSetup(csound, fft.size, FFT_FWD);
    fft.rate = anargs->srate;
    fft.data =
        (MYFLT *) csound->Malloc(csound,
                                 (anargs->fft_size + 2) * sizeof(MYFLT));

    /* main loop */
    for (frame_n = 0; frame_n < anargs->frames; frame_n++) {
      /* clear fft arrays */
      for (k = 0; k < (fft.size + 2); k++)
        fft.data[k] = (MYFLT) 0;
      /* multiply by window */
      for (k = 0; k < anargs->win_size; k++) {
        if ((filptr >= 0) && (filptr < sflen))
          fft.data[(k + first_point) % anargs->fft_size] =
              (MYFLT) window[k] * (MYFLT) bufs[0][filptr];
        filptr++;
      }
      /* we keep sample numbers of window midpoints in win_samps array */
      win_samps[frame_n] = filptr - M_2 - 1;
      /* move file pointer back */
      filptr = filptr - anargs->win_size + anargs->hop_smp;
      /* take the fft */
      csound->RealFFT(csound, setup, fft.data);
      /* peak detection */
      peaks_size = 0;
      peaks =
          peak_detection(csound, &fft, anargs->lowest_bin, anargs->highest_bin,
                         anargs->lowest_mag, norm, &peaks_size);
      /* peak tracking */
      if (peaks != NULL) {
        /* evaluate peaks SMR (masking curves) */
        evaluate_smr(peaks, peaks_size);
        if (frame_n) {
          /* initialise or update tracks */
          if ((tracks =
               update_tracks(csound, tracks, &tracks_size, anargs->track_len,
                             frame_n, ana_frames,
                             anargs->last_peak_cont)) != NULL) {
            /* do peak matching */
            unmatched_peaks =
                peak_tracking(csound, tracks, &tracks_size, peaks, &peaks_size,
                              anargs->freq_dev, 2.0 * anargs->SMR_cont,
                              &n_partials);
            /* kill unmatched peaks from previous frame */
            if (unmatched_peaks[0].peaks != NULL) {
              for (k = 0; k < unmatched_peaks[0].n_peaks; k++) {
                cpy_peak = unmatched_peaks[0].peaks[k];
                cpy_peak.amp = cpy_peak.smr = 0.0;
                peaks = push_peak(csound, &cpy_peak, peaks, &peaks_size);
              }
              csound->Free(csound, unmatched_peaks[0].peaks);
            }
            /* give birth to peaks from new frame */
            if (unmatched_peaks[1].peaks != NULL) {
              for (k = 0; k < unmatched_peaks[1].n_peaks; k++) {
                tracks =
                    push_peak(csound, &unmatched_peaks[1].peaks[k], tracks,
                              &tracks_size);
                unmatched_peaks[1].peaks[k].amp =
                    unmatched_peaks[1].peaks[k].smr = 0.0;
                ana_frames[frame_n - 1].peaks =
                    push_peak(csound, &unmatched_peaks[1].peaks[k],
                              ana_frames[frame_n - 1].peaks,
                              &ana_frames[frame_n - 1].n_peaks);
              }
              csound->Free(csound, unmatched_peaks[1].peaks);
            }
          }
          else {
            /* give number to all peaks */
            qsort(peaks, peaks_size, sizeof(ATS_PEAK), peak_frq_inc);
            for (k = 0; k < peaks_size; k++)
              peaks[k].track = n_partials++;
          }
        }
        else {
          /* give number to all peaks */
          qsort(peaks, peaks_size, sizeof(ATS_PEAK), peak_frq_inc);
          for (k = 0; k < peaks_size; k++)
            peaks[k].track = n_partials++;
        }
        /* attach peaks to ana_frames */
        ana_frames[frame_n].peaks = peaks;
        ana_frames[frame_n].n_peaks = n_partials;
        ana_frames[frame_n].time =
            (double) (win_samps[frame_n] -
                      anargs->first_smp) / (double) anargs->srate;
        /* free memory */
        csound->Free(csound, unmatched_peaks);
      }
      else {
        /* if no peaks found, initialise empty frame */
        ana_frames[frame_n].peaks = NULL;
        ana_frames[frame_n].n_peaks = 0;
        ana_frames[frame_n].time =
            (double) (win_samps[frame_n] -
                      anargs->first_smp) / (double) anargs->srate;
      }
    }
    /* free up some memory */
    csound->Free(csound, window);
    csound->Free(csound, tracks);
    csound->Free(csound, fft.data);
    /* init sound */
    csound->Message(csound, "%s", Str("Initializing ATS data..."));
    sound = (ATS_SOUND *) csound->Malloc(csound, sizeof(ATS_SOUND));
    init_sound(csound, sound, anargs->srate,
               (int) (anargs->hop_size * anargs->win_size), anargs->win_size,
               anargs->frames, anargs->duration, n_partials,
               ((anargs->type == 3 || anargs->type == 4) ? 1 : 0));
    /* store values from frames into the arrays */
    for (k = 0; k < n_partials; k++) {
      for (frame = 0; frame < sound->frames; frame++) {
        sound->time[k][frame] = ana_frames[frame].time;
        for (i = 0; i < ana_frames[frame].n_peaks; i++)
          if (ana_frames[frame].peaks[i].track == k) {
            sound->amp[k][frame] = ana_frames[frame].peaks[i].amp;
            sound->frq[k][frame] = ana_frames[frame].peaks[i].frq;
            sound->pha[k][frame] = ana_frames[frame].peaks[i].pha;
            sound->smr[k][frame] = ana_frames[frame].peaks[i].smr;
          }
      }
    }
    csound->Message(csound, "%s", Str("done!\n"));
    /* free up ana_frames memory */
    /* first, free all peaks in each slot of ana_frames... */
    for (k = 0; k < anargs->frames; k++)
      csound->Free(csound, ana_frames[k].peaks);
    /* ...then free ana_frames */
    csound->Free(csound, ana_frames);
    /* optimise sound */
    optimize_sound(csound, anargs, sound);
    /* compute residual */
    if (UNLIKELY(anargs->type == 3 || anargs->type == 4)) {
      csound->Message(csound, "%s", Str("Computing residual..."));
      compute_residual(csound, bufs, sflen, resfile, sound, win_samps,
                       anargs->srate);
      csound->Message(csound, "%s", Str("done!\n"));
    }
    /* free the rest of the memory */
    csound->Free(csound, win_samps);
    csound->Free(csound, bufs[0]);
    csound->Free(csound, bufs);
    /* analyse residual */
    if (UNLIKELY(anargs->type == 3 || anargs->type == 4)) {
#ifdef WIN32
      char buffer[160];
      char * tmp = getenv("TEMP");
      strNcpy(buffer, tmp, 160);
      // MKG 2014 Jan 29: No linkage for strlcat with MinGW here.
      // snd corrected
      //strlcat(buffer, ATSA_RES_FILE, 160);
      strncat(buffer, ATSA_RES_FILE, 159-strlen(buffer)); buffer[159]='\0';
      csound->Message(csound, "%s", Str("Analysing residual..."));
      residual_analysis(csound, buffer, sound);
#else
      csound->Message(csound, "%s", Str("Analysing residual..."));
      residual_analysis(csound, ATSA_RES_FILE, sound);
#endif
      csound->Message(csound, "%s", Str("done!\n"));
    }
    csound->Message(csound, "%s", Str("tracking completed.\n"));
    return (sound);
}

/* int32_t compute_frames(ANARGS *anargs)
 * computes number of analysis frames from the user's parameters
 * returns the number of frames
 * anargs: pointer to analysis parameters
 */
static int32_t compute_frames(ANARGS *anargs)
{
    int32_t     n_frames =
        (int) floor((float) anargs->total_samps / (float) anargs->hop_smp);

    while ((n_frames++ * anargs->hop_smp - anargs->hop_smp +
            anargs->first_smp) < (anargs->first_smp + anargs->total_samps));
    return (n_frames);
}

 /* ------------------------------------------------------------------------ */

/* private function prototypes */
static int32_t find_next_val_arr(double *arr, int32_t beg, int32_t size);
static int32_t find_next_zero_arr(double *arr, int32_t beg, int32_t size);
static int32_t find_prev_val_arr(double *arr, int32_t beg);
static void fill_sound_gaps(CSOUND *csound, ATS_SOUND *sound, int32_t min_gap_len);
static void trim_partials(CSOUND *csound, ATS_SOUND *sound, int32_t min_seg_len,
                          float min_seg_smr);
static void set_av(CSOUND *csound, ATS_SOUND *sound);

/* various conversion functions
 * to deal with dB and dB SPL
 * they take and return double floats
 */
static inline double amp2db(double amp)
{
    return (20.0 * log10(amp));
}

static inline double db2amp(double db)
{
    return (pow(10.0, db / 20.0));
}

static inline double amp2db_spl(double amp)
{
    return (amp2db(amp) + ATSA_MAX_DB_SPL);
}


/*
static inline double db2amp_spl(double db_spl)
{
    return (db2amp(db_spl - ATSA_MAX_DB_SPL));
}
*/

/* ppp2
 * ====
 * returns the closest power of two
 * greater than num
 */
static inline uint32_t ppp2(int32_t num)
{
    uint32_t tmp = 2;

    while (tmp < (unsigned int) num)
      tmp = tmp << 1;
    return (tmp);
}

/* optimize_sound
 * ==============
 * optimises an ATS_SOUND in memory before saving
 * anargs: pointer to analysis parameters
 * sound: pointer to ATS_SOUND structure
 */
static void optimize_sound(CSOUND *csound, ANARGS *anargs, ATS_SOUND *sound)
{
    double  ampmax = 0.0, frqmax = 0.0;
    int32_t     frame, partial;

    for (frame = 0; frame < sound->frames; frame++)
      for (partial = 0; partial < sound->partials; partial++) {
        if (ampmax < sound->amp[partial][frame])
          ampmax = sound->amp[partial][frame];
        if (frqmax < sound->frq[partial][frame])
          frqmax = sound->frq[partial][frame];
      }
    sound->ampmax = ampmax;
    sound->frqmax = frqmax;

    fill_sound_gaps(csound, sound, anargs->min_gap_len);
    trim_partials(csound, sound, anargs->min_seg_len, anargs->min_seg_SMR);
    set_av(csound, sound);
    /* finally set slot to 1 */
    sound->optimized = 1;
}

/* fill_sound_gaps
 * ===============
 * fills gaps in ATS_SOUND partials by interpolation
 * sound: pointer to ATS_SOUND
 * min_gap_len: minimum gap length, gaps shorter or equal to this
 * value will be filled in by interpolation
 */
static void fill_sound_gaps(CSOUND *csound, ATS_SOUND *sound, int32_t min_gap_len)
{
    int32_t     i, j, k, next_val, next_zero, prev_val, gap_size;
    double  f_inc, a_inc, s_inc, mag = TWOPI / (double) sound->srate;

    csound->Message(csound, "%s", Str("Filling sound gaps..."));
    for (i = 0; i < sound->partials; i++) {
      /* first we fix the freq gap before attack */
      next_val = find_next_val_arr(sound->frq[i], 0, sound->frames);
      if (next_val > 0) {
        for (j = 0; j < next_val; j++) {
          sound->frq[i][j] = sound->frq[i][next_val];
        }
      }
      /* fix the freq gap at end */
      prev_val = find_prev_val_arr(sound->frq[i], sound->frames - 1);
      if (prev_val != NIL && prev_val < sound->frames - 1) {
        for (j = prev_val; j < sound->frames; j++) {
          sound->frq[i][j] = sound->frq[i][prev_val];
        }
      }
      /* now we fix inner gaps of frq, pha, and amp */
      k = find_next_val_arr(sound->amp[i], 0, sound->frames);
      while (k < sound->frames && k != NIL) {
        /* find next gap: we consider gaps in amplitude, */
        /* we fix frequency and phase in parallel */
        next_zero = find_next_zero_arr(sound->amp[i], k, sound->frames);
        if (next_zero != NIL) {
          prev_val = next_zero - 1;
          next_val = find_next_val_arr(sound->amp[i], next_zero, sound->frames);
          /* check if we didn't get to end of array */
          if (next_val == NIL)
            break;
          gap_size = next_val - prev_val;
       /* csound->Message(csound,
                          "par: %d prev_val: %d next_val: %d gap_size %d\n",
                          i, prev_val, next_val, gap_size); */
          /* check validity of found gap */
          if (gap_size <= min_gap_len) {
         /* csound->Message(csound, "Filling gap of par: %d\n", i); */
            f_inc =
                (sound->frq[i][next_val] - sound->frq[i][prev_val]) / gap_size;
            a_inc =
                (sound->amp[i][next_val] - sound->amp[i][prev_val]) / gap_size;
            s_inc =
                (sound->smr[i][next_val] - sound->smr[i][prev_val]) / gap_size;
            for (j = next_zero; j < next_val; j++) {
              sound->frq[i][j] = sound->frq[i][j - 1] + f_inc;
              sound->amp[i][j] = sound->amp[i][j - 1] + a_inc;
              sound->smr[i][j] = sound->smr[i][j - 1] + s_inc;
              sound->pha[i][j] =
                  sound->pha[i][j - 1] -
                  (sound->frq[i][j] * sound->frame_size * mag);
              /* wrap phase */
              while (sound->pha[i][j] > PI) {
                sound->pha[i][j] -= TWOPI;
              }
              while (sound->pha[i][j] < (PI * (-1.0))) {
                sound->pha[i][j] += TWOPI;
              }
            }
            /* gap fixed, find next gap */
            k = next_val;
          }
          else {
            /* gap not valid, move to next one */
         /* csound->Message(csound, "jumping to next_val: %d\n", next_val); */
            k = next_val;
          }
        }
        else {
          break;
        }
      }
    }
    csound->Message(csound, "%s", Str("done!\n"));
}

/* trim_partials
 * =============
 * trim short segments from ATS_SOUND partials
 * sound: pointer to ATS_SOUND
 * min_seg_len: minimum segment length, segments shorter or equal
 * to this value will be candidates for trimming
 * min_seg_smr: minimum segment average SMR, segment candidates
 * should have an average SMR below this value to be trimmed
 */
static void trim_partials(CSOUND *csound, ATS_SOUND *sound, int32_t min_seg_len,
                          float min_seg_smr)
{
    int32_t     i, j, k, seg_beg, seg_end, seg_size, count = 0;
    double  val = 0.0, smr_av = 0.0;

    csound->Message(csound, "%s", Str("Trimming short partials..."));
    for (i = 0; i < sound->partials; i++) {
      k = 0;
      while (k < sound->frames) {
        /* find next segment */
        seg_beg = find_next_val_arr(sound->amp[i], k, sound->frames);
        if (seg_beg != NIL) {
          seg_end = find_next_zero_arr(sound->amp[i], seg_beg, sound->frames);
          /* check if we didn't get to end of array */
          if (seg_end == NIL)
            seg_end = sound->frames;
          seg_size = seg_end - seg_beg;
       /* csound->Message(csound,
                          "par: %d seg_beg: %d seg_end: %d seg_size %d\n",
                          i, seg_beg, seg_end, seg_size); */
          if (seg_size <= min_seg_len) {
            for (j = seg_beg; j < seg_end; j++) {
              if (sound->smr[i][j] > 0.0) {
                val += sound->smr[i][j];
                count++;
              }
            }
            if (count > 0)
              smr_av = val / (double) count;
            if (smr_av < min_seg_smr) {
              /* trim segment, only amplitude and SMR data */
           /* csound->Message(csound, "Trimming par: %d\n", i); */
              for (j = seg_beg; j < seg_end; j++) {
                sound->amp[i][j] = 0.0;
                sound->smr[i][j] = 0.0;
              }
            }
            k = seg_end;
          }
          else {
            /* segment not valid, move to the next one */
         /* csound->Message(csound, "jumping to seg_end: %d\n", seg_end); */
            k = seg_end;
          }
        }
        else {
          break;
        }
      }
    }
    csound->Message(csound, "%s", Str("done!\n"));
}

/* auxiliary functions to fill_sound_gaps and trim_partials */
static int32_t find_next_val_arr(double *arr, int32_t beg, int32_t size)
{
    int32_t     j, next_val = NIL;

    for (j = beg; j < size; j++)
      if (arr[j] > 0.0) {
        next_val = j;
        break;
      }
    return (next_val);
}

static int32_t find_next_zero_arr(double *arr, int32_t beg, int32_t size)
{
    int32_t     j, next_zero = NIL;

    for (j = beg; j < size; j++)
      if (arr[j] == 0.0) {
        next_zero = j;
        break;
      }
    return (next_zero);
}

static int32_t find_prev_val_arr(double *arr, int32_t beg)
{
    int32_t     j, prev_val = NIL;

    for (j = beg; j >= 0; j--)
      if (arr[j] != 0.0) {
        prev_val = j;
        break;
      }
    return (prev_val);
}

/* set_av
 * ======
 * sets the av structure slot of an ATS_SOUND,
 * it computes the average freq. and SMR for each partial
 * sound: pointer to ATS_SOUND structure
 */
static void set_av(CSOUND *csound, ATS_SOUND *sound)
{
    int32_t     i, j, count;
    double  val;

    csound->Message(csound, "%s", Str("Computing averages..."));
    for (i = 0; i < sound->partials; i++) {
      /* smr */
      val = 0.0;
      count = 0;
      for (j = 0; j < sound->frames; j++) {
        if (sound->smr[i][j] > 0.0) {
          val += sound->smr[i][j];
          count++;
        }
      }
      if (count > 0) {
        sound->av[i].smr = val / (double) count;
      }
      else {
        sound->av[i].smr = 0.0;
      }
   /* csound->Message(csound, "par: %d smr_av: %f\n",
                      i, (float)sound->av[i].smr); */
      /* frq */
      val = 0.0;
      count = 0;
      for (j = 0; j < sound->frames; j++) {
        if (sound->frq[i][j] > 0.0) {
          val += sound->frq[i][j];
          count++;
        }
      }
      if (count > 0) {
        sound->av[i].frq = val / (double) count;
      }
      else {
        sound->av[i].frq = 0.0;
      }
      /* set track# */
      sound->av[i].track = i;
    }
    csound->Message(csound, "%s", Str("done!\n"));
}

/* init_sound
 * ==========
 * initialises a new sound allocating memory
 */
static void init_sound(CSOUND *csound, ATS_SOUND *sound, int32_t sampling_rate,
                       int32_t frame_size, int32_t window_size, int32_t frames,
                       double duration, int32_t partials, int32_t use_noise)
{
    int32_t     i /* , j*/;

    if (UNLIKELY(partials==0)) {
      csound->Die(csound, "%s", Str("No partials to track -- stopping\n"));
    }
    sound->srate = sampling_rate;
    sound->frame_size = frame_size;
    sound->window_size = window_size;
    sound->frames = frames;
    sound->dur = duration;
    sound->partials = partials;
    sound->av =
        (ATS_PEAK *) csound->Malloc(csound, partials * sizeof(ATS_PEAK));
    sound->optimized = NIL;
    sound->time = (void *) csound->Malloc(csound, partials * sizeof(void *));
    sound->frq  = (void *) csound->Malloc(csound, partials * sizeof(void *));
    sound->amp  = (void *) csound->Malloc(csound, partials * sizeof(void *));
    sound->pha  = (void *) csound->Malloc(csound, partials * sizeof(void *));
    sound->smr  = (void *) csound->Malloc(csound, partials * sizeof(void *));
    sound->res  = (void *) csound->Malloc(csound, partials * sizeof(void *));
    /* allocate memory for time, amp, frq, smr, and res data */
    for (i = 0; i < partials; i++) {
      sound->time[i] =
          (double *) csound->Malloc(csound, frames * sizeof(double));
      sound->amp[i] =
          (double *) csound->Calloc(csound, frames * sizeof(double));
      sound->frq[i] =
          (double *) csound->Calloc(csound, frames * sizeof(double));
      sound->pha[i] =
          (double *) csound->Calloc(csound, frames * sizeof(double));
      sound->smr[i] =
          (double *) csound->Calloc(csound, frames * sizeof(double));
      sound->res[i] =
          (double *) csound->Calloc(csound, frames * sizeof(double));
    }
    /* init all array values with 0.0 */
    /* for (i = 0; i < partials; i++) */
    /*   for (j = 0; j < frames; j++) { */
    /*     sound->amp[i][j] = 0.0; */
    /*     sound->frq[i][j] = 0.0; */
    /*     sound->pha[i][j] = 0.0; */
    /*     sound->smr[i][j] = 0.0; */
    /*     sound->res[i][j] = 0.0; */
    /*   } */
    if (use_noise) {
      sound->band_energy =
          (double **) csound->Malloc(csound,
                                     ATSA_CRITICAL_BANDS * sizeof(double *));
      for (i = 0; i < ATSA_CRITICAL_BANDS; i++)
        sound->band_energy[i] =
            (double *) csound->Malloc(csound, frames * sizeof(double));
    }
    else
      sound->band_energy = NULL;
}

/* free_sound
 * ==========
 * frees sound's memory
 */
static void free_sound(CSOUND *csound, ATS_SOUND *sound)
{
    int32_t     k;

    if (sound != NULL) {
      csound->Free(csound, sound->av);
      /* data */
      for (k = 0; k < sound->partials; k++) {
        csound->Free(csound, sound->time[k]);
        csound->Free(csound, sound->amp[k]);
        csound->Free(csound, sound->frq[k]);
        csound->Free(csound, sound->pha[k]);
        csound->Free(csound, sound->smr[k]);
        csound->Free(csound, sound->res[k]);
      }
      /* pointers to data */
      csound->Free(csound, sound->time);
      csound->Free(csound, sound->frq);
      csound->Free(csound, sound->amp);
      csound->Free(csound, sound->pha);
      csound->Free(csound, sound->smr);
      csound->Free(csound, sound->res);
      /* check if we have residual data
       * and free its memory up
       */
      if (sound->band_energy != NULL) {
        for (k = 0; k < ATSA_CRITICAL_BANDS; k++)
          csound->Free(csound, sound->band_energy[k]);
        csound->Free(csound, sound->band_energy);
      }
      csound->Free(csound, sound);
    }
}

/* module interface */

int32_t atsa_init_(CSOUND *csound)
{
    int32_t     retval = (csound->GetUtility(csound))->AddUtility(csound, "atsa", atsa_main);

    if (!retval) {
      retval =
          (csound->GetUtility(csound))->SetUtilityDescription(csound, "atsa",
                                        Str("Soundfile analysis for ATS opcodes"));
    }
    return retval;
}

