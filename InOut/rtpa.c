/*                                               RTPA.C for PortAudio  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
    Uses PortAudio library without callbacks -- JPff
 */

#include "cs.h"
#include "soundio.h"
#include "pablio.h"

static PABLIO_Stream *pa_in = NULL, *pa_out = NULL;

static  int     ishift = 0, oshift = 0, oMaxLag;
extern  OPARMS  O;
#ifdef PIPES
#  define _pclose pclose
#endif

static int getshift(int dsize)  /* turn sample- or frame-size into shiftsize */
{
    switch(dsize) {
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
    PaError err;
    PaSampleFormat sample_type = (dsize==1?paInt8:
                                  dsize==2?paInt16:
                                  dsize==4?paFloat32:
                                  dsize==8?paPackedInt24:
                                  paCustomFormat);
    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;   /*    use the default value               */
    err = OpenAudioStream(&pa_in, (int)sr, sample_type,
                          PABLIO_READ |
                          (nchanls==1?PABLIO_MONO:PABLIO_STEREO));
    if( err != paNoError ) goto error;


    ishift = getshift(dsize);
    return;

 error:
      die(Str(X_1307,"unable to open soundcard for audio input"));
}

void playopen_(int nchanls, int dsize, float sr, int scale)
                                /* open for audio output */
{
    PaError err;
    PaSampleFormat sample_type = (dsize==1?paInt8:
                                  dsize==2?paInt16:
                                  dsize==4?paFloat32:
                                  dsize==8?paPackedInt24:
                                  paCustomFormat);
    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;   /*    use the default value               */
    err = OpenAudioStream(&pa_out, (int)sr, sample_type,
                          PABLIO_WRITE | (dsize==1?PABLIO_MONO:PABLIO_STEREO)
                          );
    if( err != paNoError ) goto error;
    
    oshift = getshift(nchanls * dsize);
    return;
 error:
    die(Str(X_1308,"unable to open soundcard for audio output"));
}

int rtrecord_(char *inbuf, int nbytes) /* get samples from ADC */
{
    ReadAudioStream( pa_in, (void*)inbuf, nbytes>>ishift);
    return(nbytes);
}

void rtplay_(void *outbuf, int nbytes) /* put samples to DAC  */
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
    WriteAudioStream(pa_out, outbuf, nbytes>>oshift);
    nrecs++;
}

void rtclose_(void)             /* close the I/O device entirely  */
{                               /* called only when both complete */
    if (pa_in!=NULL) CloseAudioStream(pa_in);
    if (pa_out!=NULL) CloseAudioStream(pa_out);
    pa_in = pa_out = NULL;
}
