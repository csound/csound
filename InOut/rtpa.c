/*                                               RTPA.C for PortAudio  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
    Uses PortAudio library without callbacks -- JPff
*/

#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"
#include "pa_blocking.h"
#include <portaudio.h>

static PA_BLOCKING_STREAM *pabsRead = 0;
static PA_BLOCKING_STREAM *pabsWrite = 0;

static  int oMaxLag;

#ifdef Str
#undef Str
#endif
#define Str(x)  (((ENVIRON*) csound)->LocalizeString(x))

typedef struct PaAlsaStreamInfo
{
  unsigned long size;
  int/*PaHostApiTypeId*/ hostApiType;
  unsigned long version;
  const char *deviceString;
}
PaAlsaStreamInfo;

/* IV - Feb 02 2005: module interface functions */

int csoundModuleCreate(void *csound)
{
    /* nothing to do, report success */
    ((ENVIRON*) csound)->Message(csound, "PortAudio real-time audio module "
                                         "for Csound by John ffitch\n");
    return 0;
}

static int playopen_(void*, csRtAudioParams*);
static int recopen_(void*, csRtAudioParams*);
static void rtplay_(void*, void*, int);
static int rtrecord_(void*, void*, int);
static void rtclose_(void*);

int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "portaudio") == 0 || strcmp(drv, "PortAudio") == 0 ||
          strcmp(drv, "portAudio") == 0 || strcmp(drv, "Portaudio") == 0 ||
          strcmp(drv, "pa") == 0 || strcmp(drv, "PA")))
      return 0;
    p->Message(csound, "rtaudio: PortAudio module enabled\n");
    p->SetPlayopenCallback(csound, playopen_);
    p->SetRecopenCallback(csound, recopen_);
    p->SetRtplayCallback(csound, rtplay_);
    p->SetRtrecordCallback(csound, rtrecord_);
    p->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

void listPortAudioDevices(void *csound)
{
    PaDeviceIndex deviceIndex = 0;
    PaDeviceIndex deviceCount = 0;
    const PaDeviceInfo *paDeviceInfo;
    deviceCount = Pa_GetDeviceCount();
    ((ENVIRON*) csound)->Message(csound, "Found %d PortAudio devices:\n",
                                         deviceCount);
    for (deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
      paDeviceInfo = Pa_GetDeviceInfo(deviceIndex);
      if (paDeviceInfo) {
        ((ENVIRON*) csound)->Message(csound, "Device%3d: %s\n",
                                             deviceIndex,
                                             paDeviceInfo->name);
        ((ENVIRON*) csound)->Message(csound,
                                     "           Maximum channels in: %7d\n",
                                     paDeviceInfo->maxInputChannels);
        ((ENVIRON*) csound)->Message(csound,
                                     "           Maximum channels out:%7d\n",
                                     paDeviceInfo->maxOutputChannels);
        ((ENVIRON*) csound)->Message(csound,
                                     "           Default sample rate: %11.3f\n",
                                     paDeviceInfo->defaultSampleRate);
      }
    }
}

static int recopen_(void *csound, csRtAudioParams *parm)
     /* open for audio input */
{
    struct PaStreamParameters paStreamParameters_;
    PaError paError = Pa_Initialize();
#if defined(LINUX)
    PaAlsaStreamInfo info;
#endif
    listPortAudioDevices(csound);
    if (paError != paNoError) goto error;
    oMaxLag = parm->bufSamp_HW; /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
#if defined(LINUX)
    printf("extdev=%p\n", parm->devName);
    if (parm->devName != NULL && strlen(parm->devName) != 0) {
      info.deviceString = parm->devName;
      ((ENVIRON*) csound)->Message(csound, "Using Portaudio input device %s.\n",
                                           parm->devName);
      info.hostApiType = paALSA;
      info.version = 1;
      info.size = sizeof(info);
      paStreamParameters_.device = paUseHostApiSpecificDeviceSpecification;
      paStreamParameters_.hostApiSpecificStreamInfo = &info;
    }
    else {
#endif
      if (parm->devNum == 1024) {
        parm->devNum = paStreamParameters_.device = Pa_GetDefaultInputDevice();
        ((ENVIRON*) csound)->Message(csound,
                                     Str("No PortAudio input device given; "
                                          "defaulting to device %d\n"),
                                     parm->devNum);
        paStreamParameters_.suggestedLatency =
          Pa_GetDeviceInfo(parm->devNum)->defaultLowInputLatency;
      }
      else {
        paStreamParameters_.device = parm->devNum;
        /* VL: dodgy... only works well with ASIO */
        paStreamParameters_.suggestedLatency = ((double) oMaxLag)
                                               / ((double) parm->sampleRate);
      }
      paStreamParameters_.hostApiSpecificStreamInfo = NULL;
#if defined(LINUX)
    }
#endif
    paStreamParameters_.channelCount = parm->nChannels;
    paStreamParameters_.sampleFormat = paFloat32;
    ((ENVIRON*) csound)->Message(csound, "Suggested PortAudio input latency = "
                                         "%f seconds.\n",
                                         paStreamParameters_.suggestedLatency);
    paError = paBlockingReadOpen(csound, &pabsRead, &paStreamParameters_,
                                 parm);
    if (paError != paNoError) goto error;
    ((ENVIRON*) csound)->Message(csound,
                                 Str("Opened PortAudio input device %i.\n"),
                                 paStreamParameters_.device);
    return 0;
 error:
    ((ENVIRON*) csound)->Message(csound, Str("PortAudio error %d: %s.\n"),
                                         paError, Pa_GetErrorText(paError));
    return -1;
}

static int playopen_(void *csound, csRtAudioParams *parm)
     /* open for audio output */
{
    struct PaStreamParameters paStreamParameters_;
    PaError paError = Pa_Initialize();
#if defined(LINUX)
    PaAlsaStreamInfo info;
#endif
    listPortAudioDevices(csound);
    if (paError != paNoError) goto error;
    oMaxLag = parm->bufSamp_HW; /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
#if defined(LINUX)
    if (parm->devName != NULL && strlen(parm->devName) != 0) {
      info.deviceString = parm->devName;
      ((ENVIRON*) csound)->Message(csound,
                                   "Using Portaudio output device %s.\n",
                                   parm->devName);
      info.hostApiType = paALSA;
      info.version = 1;
      info.size = sizeof(info);
      paStreamParameters_.device = paUseHostApiSpecificDeviceSpecification;
      paStreamParameters_.hostApiSpecificStreamInfo = &info;
    }
    else {
#endif
      if (parm->devNum == 1024) {
        parm->devNum = paStreamParameters_.device = Pa_GetDefaultOutputDevice();
        paStreamParameters_.suggestedLatency =
          Pa_GetDeviceInfo(parm->devNum)->defaultLowOutputLatency;
        ((ENVIRON*) csound)->Message(csound,
                                     Str("No PortAudio output device given; "
                                         "defaulting to device %d.\n"),
                                     paStreamParameters_.device);
      }
      else {
        paStreamParameters_.device = parm->devNum;
        /* VL: dodgy... only works well with ASIO */
        paStreamParameters_.suggestedLatency = ((double) oMaxLag)
                                               / ((double) parm->sampleRate);
      }
      paStreamParameters_.hostApiSpecificStreamInfo = NULL;
#if defined(LINUX)
    }
#endif
    paStreamParameters_.channelCount = parm->nChannels;
    paStreamParameters_.sampleFormat = paFloat32;
    ((ENVIRON*) csound)->Message(csound, "Suggested PortAudio output latency = "
                                         "%f seconds.\n",
                                         paStreamParameters_.suggestedLatency);
    paError = paBlockingWriteOpen(csound, &pabsWrite, &paStreamParameters_,
                                  parm);
    if (paError != paNoError) goto error;
    ((ENVIRON*) csound)->Message(csound,
                                 Str("Opened PortAudio output device %i.\n"),
                                 paStreamParameters_.device);
    return 0;
 error:
    ((ENVIRON*) csound)->Message(csound, Str("PortAudio error %d: %s.\n"),
                                         paError, Pa_GetErrorText(paError));
    return -1;
}

/* get samples from ADC */
static int rtrecord_(void *csound, void *inbuf_, int bytes_)
{
    paBlockingRead(pabsRead, (MYFLT *)inbuf_);
    return bytes_ / sizeof(MYFLT);
}

/* put samples to DAC  */
static void rtplay_(void *csound, void *outbuf_, int bytes_)
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
    int samples = bytes_ / sizeof(MYFLT);
    paBlockingWrite(pabsWrite, samples, (MYFLT *)outbuf_);
}

static void rtclose_(void *csound)      /* close the I/O device entirely  */
{
                                        /* called only when both complete */
    paBlockingClose(csound, pabsRead);
    paBlockingClose(csound, pabsWrite);
    Pa_Terminate();
}

