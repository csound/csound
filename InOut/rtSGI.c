/*                                                  RTAUDIO.C for SGI  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
 */

#include "cs.h"
#include "soundio.h"

#include <stdio.h>
#include <audio.h>
static  ALconfig iconfig, oconfig;
static  ALport   iport = NULL, oport = NULL;

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

void recopen_(int nchanls, int dsize, float esr, int scale)
                                /* open for audio input */
{
    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;   /*    use the default value               */
    iconfig = ALnewconfig();
    ALsetchannels(iconfig, (long)nchanls);
    ALsetwidth(iconfig, (long)dsize);
    /*      ALsetsamplerate(iconfig, (long)esr);    */
    {
      long cmdBuf[2];
      cmdBuf[0] = AL_INPUT_RATE;
      cmdBuf[1] = (long)esr;
      ALsetparams(AL_DEFAULT_DEVICE,cmdBuf,2);
    }
    ALsetqueuesize(iconfig, (long)oMaxLag);
    iport = ALopenport("soundi", "r", iconfig);
    ishift = getshift(dsize);
}

void playopen_(int nchanls, int dsize, float esr, int scale)
                                /* open for audio output */
{
    int b = 0;

    oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
    if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
      oMaxLag = IODACSAMPS;   /*    use the default value               */
    {
      int PMLqueuesize;
      oconfig = ALnewconfig();
      ALsetchannels(oconfig, (long)nchanls);
      ALsetwidth(oconfig, (long)dsize);
      /*      ALsetsamplerate(oconfig, (long)esr);    */
      {
        long cmdBuf[2];
        cmdBuf[0] = AL_OUTPUT_RATE;
        cmdBuf[1] = (long)esr;
        ALsetparams(AL_DEFAULT_DEVICE,cmdBuf,2);
      }
      ALsetqueuesize(oconfig, (long)oMaxLag);
      PMLqueuesize = ALgetqueuesize(oconfig);
      printf("\n PMLqueuesize = %d\n", PMLqueuesize);
      oport = ALopenport("soundo", "w", oconfig);
      oshift = getshift(dsize);
    }
}

int rtrecord_(char *inbuf, int nbytes) /* get samples from ADC */
{
    ALreadsamps(iport, inbuf, (long)nbytes >> ishift);
    return(nbytes);
}

void rtplay_(char *outbuf, int nbytes) /* put samples to DAC  */
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
    long sampframes = nbytes >> oshift;
    ALwritesamps(oport, outbuf, sampframes);
    nrecs++;
}

void rtclose_(void)              /* close the I/O device entirely  */
{                               /* called only when both complete */
    if (iport != NULL)
      ALcloseport(iport);
    if (oport != NULL) {
      while (ALgetfilled(oport) > 0)
        sginap(1);
      ALcloseport(oport);
    }
    if (O.Linein) {
#ifdef PIPES
      if (O.Linename[0]=='|') _pclose(Linepipe);
      else
#endif
        if (strcmp(O.Linename, "stdin")!=0) close(Linefd);
    }
}
