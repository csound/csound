/*                                               RTAUDIO.C for NeXT  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
 */

#include "cs.h"
#include "soundio.h"

#include <stdio.h>
#include <sound/sound.h>
#include <sound/sounddriver.h>
#include <mach.h>
int low_water = 48*1024;   /* Used by the driver to control the flow of samples */
int high_water = 64*1024;  /* Used by the driver to control the flow of samples */
static port_t dev_port, owner_port, write_port, reply_port ;

static  int     ishift = 0, oshift = 0, oMaxLag;
extern  OPARMS  O;

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
    die(Str(X_354,"NeXT audio record not available"));
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
      int protocol, sampleRate;
      if (esr > 33075.0)
        sampleRate = SNDDRIVER_STREAM_TO_SNDOUT_44 ;
      else sampleRate = SNDDRIVER_STREAM_TO_SNDOUT_22 ;
      if (esr != 44100.0 && esr != 22050) {
        long isr = esr, actual = isr > 33075 ? 44100 : 22050;
        if (O.msglevel & WARNMSG)
          printf(errmsg,Str(X_452,"WARNING: SRate 44.1K or 22.05K only. This %ld playing at %ld\n"),
                isr, actual);
      }
      if (nchanls != 2)
        die(Str(X_355,"NeXT supports nchnls = 2 (stereo) output only"));
      SNDAcquire(SND_ACCESS_OUT,0,0,0,NULL_NEGOTIATION_FUN,0,&dev_port,&owner_port);
      snddriver_stream_setup(dev_port, owner_port,
                             sampleRate,  /* make this a switch */
                             4096, 2, low_water, high_water, &protocol, &write_port);
      snddriver_stream_control(write_port, 0, SNDDRIVER_PAUSE_STREAM);
      /*      oshift = getshift(dsize);   */  /* what's correct here? oshift currently 0 */
    }                                   /* perhaps driver-writing takes bytes ?    */
}

int rtrecord_(char *inbuf, int nbytes) /* get samples from ADC */
{
    /* *********** Code missing ???????? ************ */
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
    snddriver_stream_start_writing(write_port,outbuf,
                                   sampframes,1,0,0,1,1,1,1,1,0, reply_port);
    if (nrecs == 3)
      snddriver_stream_control(write_port, 0, SNDDRIVER_RESUME_STREAM);
    nrecs++;
}

void rtclose_(void)              /* close the I/O device entirely  */
{                               /* called only when both complete */
    if (dev_port) {
      SNDRelease(SND_ACCESS_OUT,dev_port, owner_port) ;
      port_deallocate(task_self(),reply_port);
    }
}

