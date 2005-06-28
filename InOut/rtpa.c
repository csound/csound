
/*                                               RTPA.C for PortAudio  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
    Uses PortAudio library without callbacks -- JPff

    We will open the device full-duplex if asked to open it for
    input, regardless whether we are asked to open it for output.
    In that case we only open ONCE not twice(for input and output,
    separately) - VL
*/

#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"
#include "pa_blocking.h"
#include <portaudio.h>

#ifdef Str
#undef Str
#endif
#define Str(x)  (((ENVIRON*) csound)->LocalizeString(x))

#define INS 1
#define OUTS 0

typedef struct PaAlsaStreamInfo
{
  unsigned long size;
  int/*PaHostApiTypeId*/ hostApiType;
  unsigned long version;
  const char *deviceString;
}
PaAlsaStreamInfo;

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
    PA_BLOCKING_STREAM *pabs;
    int oMaxLag;
    int *openOnce;
    PaError paError = Pa_Initialize();
#if defined(LINUX)
    PaAlsaStreamInfo info;
#endif
    listPortAudioDevices(csound);
    if (paError != paNoError) {
    ((ENVIRON*) csound)->Message(csound, Str("PortAudio error %d: %s.\n"),
                                         paError, Pa_GetErrorText(paError));
    return -1;
    }

    oMaxLag = parm->bufSamp_HW; /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;     /*    use the default value               */
#if defined(LINUX)
    ((ENVIRON*) csound)->Message(csound, "extdev=%p\n", parm->devName);
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
    else
#endif
    {
      if (parm->devNum == 1024) {
        parm->devNum = paStreamParameters_.device = Pa_GetDefaultInputDevice();
        ((ENVIRON*) csound)->Message(csound,
                                     Str("No PortAudio input device given; "
                                          "defaulting to device %d\n"),
                                     parm->devNum);
      }
      else {
        paStreamParameters_.device = parm->devNum;
      }
      paStreamParameters_.hostApiSpecificStreamInfo = NULL;
    }
    paStreamParameters_.suggestedLatency = (double) oMaxLag
                                           / (double) parm->sampleRate;
    paStreamParameters_.channelCount = (parm->nChannels < 2 ? 2 : parm->nChannels);
    paStreamParameters_.sampleFormat = paFloat32;
    ((ENVIRON*) csound)->Message(csound, "Suggested PortAudio latency = "
                                         "%f seconds.\n",
                                         paStreamParameters_.suggestedLatency);
    /* VL: we will open the device full-duplex if asked to open it for input */
    pabs =  ((ENVIRON*) csound)->QueryGlobalVariable(csound,"pabsReadWritep");
    paError = paBlockingReadWriteOpen(csound, pabs, &paStreamParameters_, parm);
    if (paError != paNoError){
    ((ENVIRON*) csound)->Message(csound, Str("PortAudio error %d: %s.\n"),
                                         paError, Pa_GetErrorText(paError));
    return -1;
    }
    ((ENVIRON*) csound)->Message(csound,
                                 Str("Opened PortAudio full-duplex device  %i.\n"),
                                 paStreamParameters_.device);
    openOnce = (int *)((ENVIRON*) csound)->QueryGlobalVariable(csound,"openOnce");
    *openOnce = 1;
    return 0;
}

static int playopen_(void *csound, csRtAudioParams *parm)
     /* open for audio output */
{
    struct PaStreamParameters paStreamParameters_;
    PA_BLOCKING_STREAM *pabs;
    int oMaxLag;
    int *openOnce;
    PaError paError = Pa_Initialize();

#if defined(LINUX)
    PaAlsaStreamInfo info;
#endif
    listPortAudioDevices(csound);
    if (paError != paNoError) {
    ((ENVIRON*) csound)->Message(csound, Str("PortAudio error %d: %s.\n"),
                                         paError, Pa_GetErrorText(paError));
    return -1;
    }
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
    else
#endif
    {
      if (parm->devNum == 1024) {
        parm->devNum = paStreamParameters_.device = Pa_GetDefaultOutputDevice();
        ((ENVIRON*) csound)->Message(csound,
                                     Str("No PortAudio output device given; "
                                         "defaulting to device %d.\n"),
                                     paStreamParameters_.device);
      }
      else {
        paStreamParameters_.device = parm->devNum;
      }
      paStreamParameters_.hostApiSpecificStreamInfo = NULL;
    }
    paStreamParameters_.suggestedLatency = (double) oMaxLag
                                           / (double) parm->sampleRate;
    paStreamParameters_.channelCount = (parm->nChannels < 2 ? 2 : parm->nChannels);
    paStreamParameters_.sampleFormat = paFloat32;
    openOnce = (int *)((ENVIRON*) csound)->QueryGlobalVariable(csound,"openOnce");
    if(!*openOnce) {
      ((ENVIRON*) csound)->Message(csound, "Suggested PortAudio output latency = "
                                   "%f seconds.\n",
                                   paStreamParameters_.suggestedLatency);

      pabs =  (PA_BLOCKING_STREAM *)
        ((ENVIRON*) csound)->QueryGlobalVariable(csound,"pabsReadWritep");
      paError = paBlockingWriteOpen(csound, pabs, &paStreamParameters_, parm);
      if (paError != paNoError) {
        ((ENVIRON*) csound)->Message(csound, Str("PortAudio error %d: %s.\n"),
                                     paError, Pa_GetErrorText(paError));
        return -1;
      }

      ((ENVIRON*) csound)->Message(csound,
                                   Str("Opened PortAudio output device %i.\n"),
                                   paStreamParameters_.device);
    }

    return 0;

}

/* get samples from ADC */
static int rtrecord_(void *csound, void *inbuf_, int bytes_)
{
    PA_BLOCKING_STREAM *pabs;
    int samples = bytes_ / sizeof(MYFLT);
    pabs = (PA_BLOCKING_STREAM *)
      ((ENVIRON*) csound)->QueryGlobalVariableNoCheck(csound,"pabsReadWritep");
    paBlockingRead(&pabs[INS], samples,(MYFLT *)inbuf_);
    return bytes_;
}

/* put samples to DAC  */
static void rtplay_(void *csound, void *outbuf_, int bytes_)
{
    PA_BLOCKING_STREAM *pabs;
    int samples = bytes_ / sizeof(MYFLT);
    pabs = (PA_BLOCKING_STREAM *)
      ((ENVIRON*) csound)->QueryGlobalVariableNoCheck(csound,"pabsReadWritep");
    paBlockingWrite(&pabs[OUTS], samples, (MYFLT *)outbuf_);
}

static void rtclose_(void *csound)      /* close the I/O device entirely  */
{
                                        /* called only when both complete */
    paBlockingClose(csound,
                    ((ENVIRON*)csound)->QueryGlobalVariable(csound,
                                                            "pabsReadWritep"));
    /* VL: pabsReadWritep holds the whole memory block for pabsWrite &
       pabsRead on full-duplex */
    ((ENVIRON*) csound)->DestroyGlobalVariable(csound, "pabsReadWritep");
    ((ENVIRON*) csound)->DestroyGlobalVariable(csound, "openOnce");
    Pa_Terminate();
}

/* IV - Feb 02 2005: module interface functions */

PUBLIC int csoundModuleCreate(void *csound)
{
    /* nothing to do, report success */
    ((ENVIRON*) csound)->Message(csound, "PortAudio real-time audio module "
                                 "for Csound\n");
    return 0;
}

PUBLIC int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;
    int     *ptr;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "portaudio") == 0 || strcmp(drv, "PortAudio") == 0 ||
          strcmp(drv, "portAudio") == 0 || strcmp(drv, "Portaudio") == 0 ||
          strcmp(drv, "pa") == 0 || strcmp(drv, "PA") == 0))
      return 0;
    /* memory for PA_BLOCKING_STREAM dataspace for both input and output */
    p->CreateGlobalVariable(csound, "pabsReadWritep",
                            sizeof(PA_BLOCKING_STREAM)*2);
    p->CreateGlobalVariable(csound, "openOnce", sizeof(int));
    ptr = (int *)p->QueryGlobalVariable(csound,"openOnce");
    *ptr = 0;

    p->Message(csound, "rtaudio: PortAudio module enabled\n");
    p->SetPlayopenCallback(csound, playopen_);
    p->SetRecopenCallback(csound, recopen_);
    p->SetRtplayCallback(csound, rtplay_);
    p->SetRtrecordCallback(csound, rtrecord_);
    p->SetRtcloseCallback(csound, rtclose_);
    return 0;
}
