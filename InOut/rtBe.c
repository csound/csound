/*                                                      RTAUDIO.C for BeOS  */

/*  This module is included when RTAUDIO is defined at compile time.
    It provides an interface between Csound realtime record/play calls
    and the device-driver code that controls the actual hardware.
 */

#include "cs.h"
#include "soundio.h"

#include <OS.h>
#include "BeAudio.h"

static char *adcBuffer;
const bigtime_t portTimeout = 5000000;

static  int     oMaxLag;
extern  OPARMS  O;
#ifdef PIPES
#define _pclose pclose
#endif

void recopen_(int nchanls, int dsize, float esr, int scale)
                                /* open for audio input */
{
        oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
        if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
            oMaxLag = IODACSAMPS;   /*    use the default value               */
        if (nchanls != 1 && nchanls != 2) {
          die(Str(X_1154,"recopen: BeOS supports either one or two channels."));
        } else if (!(adcBuffer = malloc(oMaxLag))) {
          die(Str(X_1155,"recopen: Cannot allocate ADC buffer."));
        } else if (openADCPort(nchanls, O.informat, esr, oMaxLag) < B_NO_ERROR) {
          die(Str(X_1156,"recopen: cannot open ADC."));
        }
}

void playopen_(int nchanls, int dsize, float esr, int scale)
                                /* open for audio output */
{
        oMaxLag = O.oMaxLag;        /* import DAC setting from command line   */
        if (oMaxLag <= 0)           /* if DAC sampframes ndef in command line */
            oMaxLag = IODACSAMPS;   /*    use the default value               */
        if (nchanls != 1 && nchanls != 2) {
          die(Str(X_1128,"playopen: BeOS supports either one or two channels."));
        } else if (openDACPort(nchanls, O.outformat, esr, oMaxLag) < B_NO_ERROR) {
          die(Str(X_1129,"playopen: cannot open DAC."));
        }
}

int rtrecord_(char *inbuf, int nbytes) /* get samples from ADC */
{
        static size_t bufStart = 0;
        static size_t bufLimit = 0;

        size_t toRead = nbytes;
        while (toRead > 0) {
          size_t bufSize = bufLimit - bufStart;
          if (bufSize > toRead) {
            memcpy(inbuf, adcBuffer + bufStart, toRead);
            bufStart += toRead;
            toRead -= toRead;

          } else {
            int32 dummy;
            ssize_t nRead;

            memcpy(inbuf, adcBuffer + bufStart, bufSize);
            inbuf += bufSize;
            toRead -= bufSize;

            nRead =
              read_port_etc(gADCPort, &dummy, adcBuffer, oMaxLag,
                                                B_TIMEOUT, portTimeout);
            if (nRead < B_NO_ERROR) {
              die(Str(X_1171,"rtrecord: error reading from ADC port"));
            }
            bufStart = 0;
            bufLimit = (size_t)nRead;
          }
        }
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
    /* (the resolution of all software-synthesised events), the user can      */
    /* eliminate MIDI jitter by requesting that both be made synchronous with */
    /* the above audio I/O blocks, i.e. by setting -b to some 1 or 2 K-prds.  */
{
        while (nbytes > 0) {
          size_t toWrite = (nbytes < oMaxLag) ? nbytes : oMaxLag;
          if (write_port_etc(gDACPort, 0, (char*)outbuf, toWrite,
                             B_TIMEOUT, portTimeout) < B_NO_ERROR) {
            printf(Str(X_1170,"rtplay: failed write to DAC port\n"));
          }
          outbuf += toWrite;
          nbytes -= toWrite;
        }
        nrecs++;
}

void rtclose_(void)              /* close the I/O device entirely  */
{                               /* called only when both complete */
        closeADCPort();
        closeDACPort();
        if (O.Linein) {
#ifdef PIPES
          if (O.Linename[0]=='|') _pclose(Linepipe);
          else
#endif
          if (strcmp(O.Linename, "stdin")!=0) close(Linefd);
        }
}
