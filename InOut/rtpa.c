/*                                               RTPA.C for PortAudio  */

/*  This module is included when RTAUDIO is defined at compile time.
It provides an interface between Csound realtime record/play calls
and the device-driver code that controls the actual hardware.
Uses PortAudio library without callbacks -- JPff
*/

#include "cs.h"
#include "soundio.h"
#include <portaudio.h>

extern char    *sfoutname;                     /* soundout filename    */
extern char    *inbuf;
extern char    *outbuf;                        /* contin sndio buffers */
extern char    *chinbufp, *choutbufp;          /* char  pntr to above  */
extern short   *shinbufp, *shoutbufp;          /* short pntr           */
extern long    *llinbufp, *lloutbufp;          /* long  pntr           */
extern float   *flinbufp, *floutbufp;          /* MYFLT pntr           */
extern unsigned inbufrem, outbufrem;           /* in monosamps         */
                                               /* (see openin,iotranset)    */
extern unsigned inbufsiz,  outbufsiz;          /* alloc in sfopenin/out     */
extern int     isfd, isfopen, infilend;        /* (real set in sfopenin)    */
extern int     osfd, osfopen;                  /* (real set in sfopenout)   */
extern int     pipdevin, pipdevout;            /* mod by sfopenin,sfopenout */
extern unsigned long  nframes;

extern void (*spinrecv)(void);
extern void (*spoutran)(void);
extern void (*nzerotran)(long);
extern void spoutsf(void);
extern void zerosf(long len);
extern void (*audtran)(void *, int);
static void szerotran(long kcnt);
void rtplay_(void *outbuf, int nbytes);

static PaStream *pa_in = NULL, *pa_out = NULL;

static  int ishift = 0, oshift = 0, oMaxLag;
extern  OPARMS  O;
#ifdef PIPES
#  define _pclose pclose
#endif

static int getshift(int dsize)  /* turn sample- or frame-size into shiftsize */
{
    switch (dsize) {
    case 1:  return(0);
    case 2:  return(1);
    case 4:  return(2);
    case 8:  return(3);
    default: die(Str(X_1169,"rtaudio: illegal dsize"));
      return(-1);           /* Not reached */
    }
}

void recopen_(int nchanls, int dsize, float sr, int scale)
/* open for audio input */
{
    struct PaStreamParameters paStreamParameters_;
    PaError paError;
    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
    paStreamParameters_.device = 0;
    paStreamParameters_.channelCount = nchnls;
    paStreamParameters_.sampleFormat = (dsize==1?paInt8:
                                        dsize==2?paInt16:
                                        dsize==4?paFloat32:
                                        dsize==8?paInt24:
                                        paCustomFormat);
    paStreamParameters_.suggestedLatency = ((double) sr) / ((double) oMaxLag);
    paStreamParameters_.hostApiSpecificStreamInfo = 0;
    paError = Pa_OpenStream (&pa_in,
                             &paStreamParameters_,
                             0,
                             (double) sr,
                             (unsigned long) oMaxLag,
                             0,
                             0,
                             0);
    if( paError != paNoError )
      goto error;
    ishift = getshift(dsize);
    return;
 error:
    die(Str(X_1307,"unable to open soundcard for audio input"));
}

void listPortAudioDevices() {
    PaDeviceIndex deviceIndex = 0;
    PaDeviceIndex deviceCount = 0;
    PaDeviceInfo *paDeviceInfo;

    deviceCount = Pa_GetDeviceCount();

    for(deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
      paDeviceInfo = Pa_GetDeviceInfo(deviceIndex);
      if(paDeviceInfo) {
        err_printf("PortAudio device %d\n  %s\n  Maximum channels in:  "
                   "%5d\n  Maximum channels out: %5d\n  Default sample rate: %10.3f\n",
                   deviceIndex,
                   paDeviceInfo->name,
                   paDeviceInfo->maxInputChannels,
                   paDeviceInfo->maxOutputChannels,
                   paDeviceInfo->defaultSampleRate);
      }
    }
}

void playopen_(int nchnls_, int dsize_, float sr_, int scale_)
                                                  /* open for audio output */
{
    struct PaStreamParameters paStreamParameters_;
    PaError paError = Pa_Initialize();
    if ( paError != paNoError )
      goto error;

    listPortAudioDevices();

    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */

    if (rtout_dev == 1024) {
      paStreamParameters_.device = 1;
      err_printf("No PortAudio device given.  Defaulting to device 1.\n");
    }
    else {
      paStreamParameters_.device = rtout_dev;
      err_printf("Using Portaudio Device %i\n", rtout_dev);
    }

    paStreamParameters_.channelCount = nchnls_;
    paStreamParameters_.sampleFormat = paFloat32;
    paStreamParameters_.suggestedLatency = ((double) sr_) / ((double) oMaxLag);
    paStreamParameters_.hostApiSpecificStreamInfo = 0;

    paError = Pa_OpenStream (&pa_out,
                             0,
                             &paStreamParameters_,
                             (double) sr_,
                             (unsigned long) oMaxLag,
                             0,
                             0,
                             0);

    if ( paError != paNoError )
      goto error;

    paError = Pa_StartStream( pa_out );
    audtran = rtplay_;

    if ( paError != paNoError )
      goto error;

    spoutran = spoutsf;       /* accumulate output */
    nzerotran = zerosf;       /* quick zeros */
    audtran = rtplay_;        /* flush buffer */
    outbufrem = O.outbufsamps;
    osfopen = 1;
    return;
 error:
    err_printf("PortAudio error %d: %s\n", paError, Pa_GetErrorText(paError));
    die(Str(X_1308,"unable to open soundcard for audio output"));
}


int rtrecord_(char *inbuf, int nbytes) /* get samples from ADC */
{
    Pa_ReadStream(pa_in, (void*)inbuf, nbytes>>ishift);
    return(nbytes);
}

void rtplay_(void *outbuf_, int bytes_) /* put samples to DAC  */
/* N.B. This routine serves as a THROTTLE in Csound Realtime Performance, */
/* delaying the actual writes and return until the hardware output buffer */
/* passes a sample-specific THRESHOLD.  If the I/O BLOCKING functionality */
/* is implemented ACCURATELY by the vendor-supplied audio-library write,  */
/* that is sufficient.  Otherwise, requires some kind of IOCTL from here. */
/* This functionality is IMPORTANT when other realtime I/O is occurring,  */
/* such as when external MIDI data is being collected from a serial port. */
/* Since Csound polls for MIDI input at the software synthesis K-rate     */
/* (the resolution of all software-synthesized events), the user can      */
/* eliminate MIDI jitter by requesting that both be made synchronous with */
/* the above audio I/O blocks, i.e. by setting -b to some 1 or 2 K-prds.  */
{
    int samples = (bytes_>>oshift) / sizeof(MYFLT);
    int frames = samples / nchnls;
#if defined(USE_DOUBLE)
    MYFLT *myfltBuffer = (MYFLT *)outbuf_;
    int i;
    float actualBuffer[samples];
    for (i = 0; i < samples; ++i) {
      actualBuffer[i] = myfltBuffer[i];
    }
    PaError paError = Pa_WriteStream(pa_out, actualBuffer, frames);
#else
    PaError paError = Pa_WriteStream(pa_out, outbuf_, frames);
#endif
    if ( paError != paNoError )
      err_printf("PortAudio error %d: %s\n", paError, Pa_GetErrorText(paError));
    nrecs++;
}

void rtclose_(void)             /* close the I/O device entirely  */
{                               /* called only when both complete */
    if (pa_in)
      Pa_AbortStream(pa_in);
    if (pa_out)
      Pa_AbortStream(pa_out);
    pa_in = pa_out = 0;
}

