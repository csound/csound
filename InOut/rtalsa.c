
/* ALSA real-time audio routines -- written by Istvan Varga, Jan 2005 */

/* FIXME: use new API */
#define ALSA_PCM_OLD_HW_PARAMS_API 1
#define ALSA_PCM_OLD_SW_PARAMS_API 1

#include <alsa/asoundlib.h>
/* no #ifdef, always have it on Linux */
#include <unistd.h>
#include "cs.h"
#include "soundio.h"

#ifdef PIPES
#  define _pclose pclose
#endif

/* FIXME */
#ifdef warning
#undef warning
#endif
#define warning(x) { if (O.msglevel & WARNMSG) err_printf("%s\n", x); }

typedef struct devparams_ {
        char                *device;            /* device name              */
        snd_pcm_format_t    format;             /* sample format            */
        int                 sample_size;        /* sample size in bytes     */
        int                 period_size;        /* sample buffer size       */
        int                 srate;              /* sample rate in Hz        */
        int                 nchns;              /* number of channels       */
        snd_pcm_sframes_t   buffer_smps;        /* buffer length in samples */
        snd_pcm_sframes_t   period_smps;        /* period time in samples   */
        snd_pcm_t           *handle;            /* handle                   */
        snd_pcm_hw_params_t *hw_params;         /* hardware parameters      */
        snd_pcm_sw_params_t *sw_params;         /* software parameters      */
        snd_pcm_access_t    access;             /* access method            */
} DEVPARAMS;

static DEVPARAMS    in_dev_, out_dev_;  /* input and output device          */
static DEVPARAMS    *in_dev = NULL, *out_dev = NULL;

/* select sample format */

static void set_format(DEVPARAMS *dev, int csound_format)
{
    short   endian_test = 0x1234;

    if (*((unsigned char*) (&endian_test)) == (unsigned char) 0x34) {
      /* little-endian */
      switch (csound_format) {
        case AE_CHAR:   dev->format = SND_PCM_FORMAT_S8;              break;
        case AE_ALAW:   dev->format = SND_PCM_FORMAT_A_LAW;           break;
        case AE_ULAW:   dev->format = SND_PCM_FORMAT_MU_LAW;          break;
        case AE_SHORT:  dev->format = SND_PCM_FORMAT_S16_LE;          break;
        case AE_LONG:   dev->format = SND_PCM_FORMAT_S32_LE;          break;
        case AE_FLOAT:  dev->format = SND_PCM_FORMAT_FLOAT_LE;        break;
        case AE_UNCH:   dev->format = SND_PCM_FORMAT_U8;              break;
        case AE_24INT:  dev->format = SND_PCM_FORMAT_S24_LE;          break;
        case AE_DOUBLE: dev->format = SND_PCM_FORMAT_FLOAT64_LE;      break;
        default:        die(Str("unknown sample format"));            break;
      }
    }
    else {
      /* big-endian */
      switch (csound_format) {
        case AE_CHAR:   dev->format = SND_PCM_FORMAT_S8;              break;
        case AE_ALAW:   dev->format = SND_PCM_FORMAT_A_LAW;           break;
        case AE_ULAW:   dev->format = SND_PCM_FORMAT_MU_LAW;          break;
        case AE_SHORT:  dev->format = SND_PCM_FORMAT_S16_BE;          break;
        case AE_LONG:   dev->format = SND_PCM_FORMAT_S32_BE;          break;
        case AE_FLOAT:  dev->format = SND_PCM_FORMAT_FLOAT_BE;        break;
        case AE_UNCH:   dev->format = SND_PCM_FORMAT_U8;              break;
        case AE_24INT:  dev->format = SND_PCM_FORMAT_S24_BE;          break;
        case AE_DOUBLE: dev->format = SND_PCM_FORMAT_FLOAT64_BE;      break;
        default:        die(Str("unknown sample format"));            break;
      }
    }
    /* calculate sample size */
    switch (csound_format) {
      case AE_CHAR:                                     /* 8-bit formats */
      case AE_UNCH:
      case AE_ALAW:
      case AE_ULAW:   dev->sample_size = 1; break;
      case AE_SHORT:  dev->sample_size = 2; break;      /* 16-bit formats */
      case AE_24INT:  dev->sample_size = 3; break;      /* 24-bit formats */
      case AE_LONG:                                     /* 32-bit formats */
      case AE_FLOAT:  dev->sample_size = 4; break;
      case AE_DOUBLE: dev->sample_size = 8; break;      /* 64-bit formats */
    }
}

/* set up audio device */

static void set_device_params(DEVPARAMS *dev, char *device_name, int output,
                              int csound_format, float sr, int chn)
{
    int     err, dir, n;
    char    msg[1024];

    /* open the device ... */
    if (!device_name) device_name = "default";
    if (output) {
      /* ... for playback */
      err = snd_pcm_open(&(dev->handle), device_name,
                         SND_PCM_STREAM_PLAYBACK, 0);
      if (err < 0) {
        sprintf(msg, "cannot open device %s for audio output: %s",
                     device_name, snd_strerror(err));
        die(msg);
      }
    }
    else {
      /* ... for capture */
      err = snd_pcm_open(&(dev->handle), device_name,
                         SND_PCM_STREAM_CAPTURE, 0);
      if (err < 0) {
        sprintf(msg, "cannot open device %s for audio input: %s",
                     device_name, snd_strerror(err));
        die(msg);
      }
    }
    /* allocate hardware and software parameters */
    snd_pcm_hw_params_alloca(&(dev->hw_params));
    snd_pcm_sw_params_alloca(&(dev->sw_params));
    if (snd_pcm_hw_params_any(dev->handle, dev->hw_params) < 0)
      die("no real-time audio configurations found");

    /* now set the various hardware parameters: */
    /* access method, */
    dev->access = SND_PCM_ACCESS_RW_INTERLEAVED;
    if (snd_pcm_hw_params_set_access(dev->handle, dev->hw_params, dev->access)
        < 0)
      die("error setting access type for soundcard");
    /* sample format, */
    set_format(dev, csound_format);
    if (snd_pcm_hw_params_set_format(dev->handle, dev->hw_params, dev->format)
        < 0)
      die(Str("unable to set requested sample format on soundcard"));
    /* number of channels, */
    dev->nchns = chn;
    if (snd_pcm_hw_params_set_channels(dev->handle, dev->hw_params, chn) < 0)
      die(Str("unable to set mode (mono/stereo) on soundcard"));
    dev->sample_size *= chn;            /* correct sample size */
    /* sample rate, */
    dev->srate =
      (int) snd_pcm_hw_params_set_rate_near(dev->handle, dev->hw_params,
                                            (int) (sr + 0.5f), 0);
    if (dev->srate < 0)
      die(Str("unable to set sample rate on soundcard"));
    if (dev->srate != (int) (sr + 0.5f)) {
      sprintf(msg, Str("Sample rate set to %d (instead of %d)\n"),
                   dev->srate, (int) (sr + 0.5f));
      err_printf(msg);
    }
    /* buffer size, */
    n = (O.oMaxLag >= 16 ? O.oMaxLag : 1024);
    dev->buffer_smps = (snd_pcm_sframes_t)
      snd_pcm_hw_params_set_buffer_size_near(dev->handle, dev->hw_params,
                                             (snd_pcm_uframes_t) n);
    if (dev->buffer_smps <= (snd_pcm_sframes_t) 0)
      die(Str("failed while trying to set soundcard DMA buffer size"));
    if ((int) dev->buffer_smps != n) {
      sprintf(msg, "Soundcard DMA buffer size set to %d samples "
                   "(instead of %d)\n",
              (int) dev->buffer_smps, n);
      err_printf(msg);
    }
    /* and period size */
    n = (output ? O.outbufsamps : O.inbufsamps) / nchnls;
    dev->period_smps = (snd_pcm_sframes_t)
      snd_pcm_hw_params_set_period_size_near(dev->handle, dev->hw_params,
                                             (snd_pcm_uframes_t) n, &dir);
    if (dev->period_smps <= 0)
      die("error setting period time for real-time audio");
    if ((int) dev->period_smps != n) {
      sprintf(msg, "Real-time audio period size set to %d samples "
                   "(instead of %d)\n",
              (int) dev->period_smps, n);
      err_printf(msg);
    }
    dev->period_size = (int) dev->period_smps * dev->sample_size * chn;
    /* set up device according to the above parameters */
    if (snd_pcm_hw_params(dev->handle, dev->hw_params) < 0)
      die("error setting hardware parameters for real-time audio");

    /* now set software parameters */
    n = (output ? dev->buffer_smps : 1);
    if (snd_pcm_sw_params_current(dev->handle, dev->sw_params) < 0
        || snd_pcm_sw_params_set_start_threshold(dev->handle, dev->sw_params,
                                                 (snd_pcm_uframes_t) n) < 0
        || snd_pcm_sw_params_set_avail_min(dev->handle, dev->sw_params,
                                           dev->period_smps) < 0
        || snd_pcm_sw_params_set_xfer_align(dev->handle, dev->sw_params, 1) < 0
        || snd_pcm_sw_params(dev->handle, dev->sw_params) < 0)
      die("error setting software parameters for real-time audio");
}

/* open for audio input */

void recopen_(int nchnls_, int dsize_, float sr_, int scale_)
{
    IGN(dsize_);
    IGN(scale_);
    if (O.informat != AE_FLOAT)
      die(Str(" *** cannot record sample formats other than float"));
    else {
      in_dev = &in_dev_;
      /* FIXME: allow device selection */
      set_device_params(in_dev, NULL, 0, O.informat, sr_, nchnls_);
    }
}

/* open for audio output */

void playopen_(int nchnls_, int dsize_, float sr_, int scale_)
{
    IGN(dsize_);
    IGN(scale_);
    if (O.outformat != AE_FLOAT)
      die(Str(" *** cannot play sample formats other than float"));
    else {
      out_dev = &out_dev_;
      /* FIXME: allow device selection */
      set_device_params(out_dev, NULL, 1, O.outformat, sr_, nchnls_);
    }
}

/* get samples from ADC */

int rtrecord_(void *inbuf_, int bytes_)
{
    int     n = bytes_ / in_dev_.sample_size;   /* number of samples */
    int     m = 0, err;

    while (n) {
      err = (int) snd_pcm_readi(in_dev_.handle, (void*) inbuf_,
                                (snd_pcm_uframes_t) n);
      if (err >= 0) {
        n -= err; m += err; continue;
      }
      /* handle I/O errors */
      if (err == -EPIPE) {
        /* buffer underrun */
        warning(Str("Buffer underrun in real-time audio input")); /* complain */
        if (snd_pcm_prepare(in_dev_.handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning(Str("Real-time audio input suspended"));
        while (snd_pcm_resume(in_dev_.handle) == -EAGAIN) sleep(1);
        if (snd_pcm_prepare(in_dev_.handle) >= 0) continue;
      }
      /* could not recover from error */
      die(Str("Error reading data from audio input device"));
    }
    return (m * in_dev_.sample_size);
}

/* put samples to DAC */

void rtplay_(void *outbuf_, int bytes_)
{
    int     n = bytes_ / out_dev_.sample_size;  /* number of samples */
    int     err;

    nrecs++;
    while (n) {
      err = (int) snd_pcm_writei(out_dev_.handle, (void*) outbuf_,
                                 (snd_pcm_uframes_t) n);
      if (err >= 0) {
        n -= err; continue;
      }
      /* handle I/O errors */
      if (err == -EPIPE) {
        /* buffer underrun */
        warning(Str("Buffer underrun in real-time audio output"));/* complain */
        if (snd_pcm_prepare(out_dev_.handle) >= 0) continue;
      }
      else if (err == -ESTRPIPE) {
        /* suspend */
        warning(Str("Real-time audio output suspended"));
        while (snd_pcm_resume(out_dev_.handle) == -EAGAIN) sleep(1);
        if (snd_pcm_prepare(out_dev_.handle) >= 0) continue;
      }
      /* could not recover from error */
      die(Str("Error writing data to audio output device"));
    }
}

/* close the I/O device entirely  */
/* called only when both complete */

void rtclose_(void)
{
    if (in_dev) {
      snd_pcm_close(in_dev->handle);
      in_dev = NULL;
    }
    if (out_dev) {
      snd_pcm_close(out_dev->handle);
      out_dev = NULL;
    }
#if 0
    if (O.Linein) {
#ifdef PIPES
      if (O.Linename[0]=='|') _pclose(Linepipe);
      else
#endif
        if (strcmp(O.Linename, "stdin") != 0) close(Linefd);
    }
#endif
}

