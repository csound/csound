/*                                               RTPA.C for PortAudio  */

/*  This module is included when RTAUDIO is defined at compile time.
It provides an interface between Csound realtime record/play calls
and the device-driver code that controls the actual hardware.
Uses PortAudio library without callbacks -- JPff
*/

#include "cs.h"
#include "soundio.h"
#include "pa_blocking.h"
#include <portaudio.h>

#ifdef MSVC
#include <windows.h>

#endif


extern char    *sfoutname;                     /* soundout filename    */
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
extern int (*audrecv)(void *, int);
void rtplay_(void *outbuf, int nbytes);
int rtrecord_(void *inbuf_, int bytes_);

#if !defined(WIN32)
static PaStream *pa_in = NULL, *pa_out = NULL;
#endif

#if defined(WIN32)
static PA_BLOCKING_STREAM *pabsRead = 0;
static PA_BLOCKING_STREAM *pabsWrite = 0;
#endif

static  int oMaxLag;
extern  OPARMS  O;
#ifdef PIPES
#  define _pclose pclose
#endif

typedef struct PaAlsaStreamInfo
{
    unsigned long size;
    int/*PaHostApiTypeId*/ hostApiType;
    unsigned long version;
    const char *deviceString;
}
PaAlsaStreamInfo;

void listPortAudioDevices(void)
{
    PaDeviceIndex deviceIndex = 0;
    PaDeviceIndex deviceCount = 0;
    const PaDeviceInfo *paDeviceInfo;
    deviceCount = Pa_GetDeviceCount();
    err_printf("Found %d PortAudio devices:\n", deviceCount);
    for (deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
      paDeviceInfo = Pa_GetDeviceInfo(deviceIndex);
      if (paDeviceInfo) {
        err_printf("Device%3d: %s\n",
                   deviceIndex,
                   paDeviceInfo->name);
        err_printf("           Maximum channels in: %7d\n",
                   paDeviceInfo->maxInputChannels);
        err_printf("           Maximum channels out:%7d\n",
                   paDeviceInfo->maxOutputChannels);
        err_printf("           Default sample rate: %11.3f\n",
                   paDeviceInfo->defaultSampleRate);
      }
    }
}

void recopen_(int nchnls_, int dsize_, float sr_, int scale_)
/* open for audio input */
{
    struct PaStreamParameters paStreamParameters_;
    PaError paError = Pa_Initialize();
#if defined(LINUX)
    PaAlsaStreamInfo info;
#endif
    if (paError != paNoError) goto error;
    listPortAudioDevices();
    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
#if defined(LINUX)
    printf("extdev=%p\n", rtin_devs);
    if (rtin_devs!=NULL && strlen(rtin_devs)!=0) {
      info.deviceString = rtin_devs;
      err_printf("Using Portaudio input device %s.\n", rtin_devs);
      info.hostApiType = paALSA;
      info.version = 1;
      info.size = sizeof(info);
      paStreamParameters_.device = paUseHostApiSpecificDeviceSpecification;
      paStreamParameters_.hostApiSpecificStreamInfo = &info;
    }
    else {
#endif
      if (rtin_dev == 1024) {
        rtin_dev = paStreamParameters_.device = Pa_GetDefaultInputDevice();
        err_printf(Str(X_30,
                       "No PortAudio input device given; "
                       "defaulting to device %d\n"), rtin_dev);
        paStreamParameters_.suggestedLatency =
          Pa_GetDeviceInfo(rtin_dev)->defaultLowInputLatency;
      }
      else {
        paStreamParameters_.device = rtin_dev;
                        /* VL: dodgy... only works well with ASIO */
        paStreamParameters_.suggestedLatency = ((double) oMaxLag) / ((double) sr_);
      }
      paStreamParameters_.hostApiSpecificStreamInfo = NULL;
#if defined(LINUX)
    }
#endif
    paStreamParameters_.channelCount = nchnls_;
    paStreamParameters_.sampleFormat = paFloat32;
        /* VL: moved to five lines above */
    /*paStreamParameters_.suggestedLatency = ((double) oMaxLag) / ((double) sr_);*/
    err_printf("Suggested PortAudio input latency = %f seconds.\n",
               paStreamParameters_.suggestedLatency);
#if defined(WIN32)
    paError = paBlockingReadOpen(&cenviron,
                                 &pabsRead,
                                 &paStreamParameters_);
    if (paError != paNoError) goto error;
#else
    paError = Pa_OpenStream(&pa_in,
                            &paStreamParameters_,
                            NULL,
                            (double) sr_,
                            (unsigned long) oMaxLag /* 0ul */,
                            paNoFlag,
                            NULL,
                            NULL);
    if (paError != paNoError) goto error;
    paError = Pa_StartStream(pa_in);
    if (paError != paNoError) goto error;
#endif
    audrecv = rtrecord_;
    /* spinrecv = spinsf;    */   /* accumulate output */
    /* nzerotran = zerosf;   */    /* quick zeros */
    inbufrem = O.inbufsamps;
    isfopen = 1;
    err_printf(Str(X_39,"Opened PortAudio input device %i.\n"),
        paStreamParameters_.device);
    return;
 error:
    err_printf(Str(X_41,"PortAudio error %d: %s.\n"),
               paError, Pa_GetErrorText(paError));
    die(Str(X_1307,"Unable to open PortAudio input device."));
}

void playopen_(int nchnls_, int dsize_, float sr_, int scale_)
                                                  /* open for audio output */
{
    struct PaStreamParameters paStreamParameters_;
    PaError paError = Pa_Initialize();
#if defined(LINUX)
    PaAlsaStreamInfo info;
#endif
    if (paError != paNoError) goto error;
    listPortAudioDevices();
    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
#if defined(LINUX)
    if (rtout_devs!=NULL && strlen(rtout_devs)!=0) {
      info.deviceString = rtout_devs;
      err_printf("Using Portaudio output device %s.\n", rtout_devs);
      info.hostApiType = paALSA;
      info.version = 1;
      info.size = sizeof(info);
      paStreamParameters_.device = paUseHostApiSpecificDeviceSpecification;
      paStreamParameters_.hostApiSpecificStreamInfo = &info;
    }
    else {
#endif
      if (rtout_dev == 1024) {
        rtout_dev = paStreamParameters_.device = Pa_GetDefaultOutputDevice();
        paStreamParameters_.suggestedLatency =
          Pa_GetDeviceInfo(rtout_dev)->defaultLowOutputLatency;
        err_printf(Str(X_30,
                       "No PortAudio output device given; "
                       "defaulting to device %d.\n"),
                   paStreamParameters_.device);
      }
      else {
        paStreamParameters_.device = rtout_dev;
        /* VL: dodgy... only works well with ASIO */
        paStreamParameters_.suggestedLatency = ((double) oMaxLag) / ((double) sr_);
      }
      paStreamParameters_.hostApiSpecificStreamInfo = NULL;
#if defined(LINUX)
    }
#endif
    paStreamParameters_.channelCount = nchnls_;
    paStreamParameters_.sampleFormat = paFloat32;
    err_printf("Suggested PortAudio output latency = %f seconds.\n",
               paStreamParameters_.suggestedLatency);
#if defined(WIN32)
    paError = paBlockingWriteOpen(&cenviron,
                                  &pabsWrite,
                                  &paStreamParameters_);
    if (paError != paNoError) goto error;
#else
    paError = Pa_OpenStream(&pa_out,
                            NULL,
                            &paStreamParameters_,
                            (double) sr_,
                            (unsigned long) oMaxLag /* 0ul */,
                            paNoFlag,
                            NULL,
                            NULL);
    if (paError != paNoError) goto error;
    paError = Pa_StartStream(pa_out);
    if (paError != paNoError) goto error;
#endif
    audtran = rtplay_;
    spoutran = spoutsf;       /* accumulate output */
    nzerotran = zerosf;       /* quick zeros */
    outbufrem = O.outbufsamps;
    osfopen = 1;
    err_printf(Str(X_39,"Opened PortAudio output device %i.\n"),
               paStreamParameters_.device);
    return;
 error:
    err_printf(Str(X_41,"PortAudio error %d: %s.\n"),
               paError, Pa_GetErrorText(paError));
    die(Str(X_1308,"Unable to open PortAudio output device."));
}

int rtrecord_(void *inbuf_, int bytes_) /* get samples from ADC */
{
#if defined(WIN32)
    paBlockingRead(pabsRead, (MYFLT *)inbuf_);
    return bytes_ / sizeof(MYFLT);
#else
    int samples = bytes_ / sizeof(MYFLT);
    int frames = samples / nchnls;
#if defined(USE_DOUBLE)
    float actualBuffer[samples];
    PaError paError = Pa_ReadStream(pa_in, actualBuffer, frames);
    MYFLT *myfltBuffer = (MYFLT *)inbuf_;
    int i;
    for (i = 0; i < samples; ++i) {
      myfltBuffer[i] = (MYFLT)actualBuffer[i];
    }
#else
    PaError paError = Pa_ReadStream(pa_in, inbuf_, frames);
#endif
    if (paError != paNoError) {
      err_printf(Str(X_41,"PortAudio error %d: %s\n"),
                 paError, Pa_GetErrorText(paError));
      return 0;
    }
    else {
      return samples;
    }
#endif
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
#if defined(WIN32)
    paBlockingWrite(pabsWrite, (MYFLT *)outbuf_);
#else
    int samples = bytes_ / sizeof(MYFLT);
    int frames = samples / nchnls;
#if defined(USE_DOUBLE)
    MYFLT *myfltBuffer = (MYFLT *)outbuf_;
    int i;
    float actualBuffer[samples];
    for (i = 0; i < samples; ++i) {
      actualBuffer[i] = (float)myfltBuffer[i];
    }
    PaError paError = Pa_WriteStream(pa_out, actualBuffer, frames);
#else
    PaError paError = Pa_WriteStream(pa_out, outbuf_, frames);
#endif
    if (paError != paNoError)
      err_printf(Str(X_41,"PortAudio error %d: %s\n"),
                 paError, Pa_GetErrorText(paError));
    nrecs++;
#endif
}

void rtclose_(void)             /* close the I/O device entirely  */
{                               /* called only when both complete */
#if defined(WIN32)
    paBlockingClose(pabsRead);
    paBlockingClose(pabsWrite);
#else
    if (pa_in)
      Pa_AbortStream(pa_in);
    if (pa_out)
      Pa_AbortStream(pa_out);
    pa_in = pa_out = 0;
#endif
#ifndef MSVC /* VL MSVC fix */
    sleep(1);
#else
    Sleep(1000);
#endif

}

