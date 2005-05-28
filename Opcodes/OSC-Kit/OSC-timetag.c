/*
Copyright (c) 1998.  The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

The OpenSound Control WWW page is
    http://www.cnmat.berkeley.edu/OpenSoundControl
*/

/*

 OSC_timeTag.c: library for manipulating OSC time tags
 Matt Wright, 5/29/97

 Version 0.2 (9/11/98): cleaned up so no explicit type names in the .c file.

*/

#include "OSC-timetag.h"

#ifdef HAS8BYTEINT
#define TWO_TO_THE_32_FLOAT 4294967296.0f

OSCTimeTag OSCTT_Immediately(void) {
    return (OSCTimeTag) 1;
}

OSCTimeTag OSCTT_BiggestPossibleTimeTag(void) {
    return (OSCTimeTag) 0xffffffffffffffff;
}

OSCTimeTag OSCTT_PlusSeconds(OSCTimeTag original, float secondsOffset) {
    int8 offset = (int8) (secondsOffset * TWO_TO_THE_32_FLOAT);

  /*    printf("* OSCTT_PlusSeconds %llx plus %f seconds (i.e., %lld offset) is %llx\n", original,
              secondsOffset, offset, original + offset);  */

    return original + offset;
}

int OSCTT_Compare(OSCTimeTag left, OSCTimeTag right) {
#if 0
    printf("***** OSCTT_Compare(%llx, %llx): %d\n", left, right,
           (left<right) ? -1 : ((left == right) ? 0 : 1));
#endif
    if (left < right) {
        return -1;
    } else if (left == right) {
        return 0;
    } else {
        return 1;
    }
}

void OSCTT_SetFromInt(OSCTimeTag *self, unsigned int value) {
       if (self != (OSCTimeTag *) NULL)
               *self = value;
}

#ifdef __sgi
#include <sys/time.h>

#define SECONDS_FROM_1900_to_1970 2208988800UL /* 17 leap years */
#define TWO_TO_THE_32_OVER_ONE_MILLION 4295

OSCTimeTag OSCTT_CurrentTime(void) {
    uint8 result;
    uint4 usecOffset;
    struct timeval tv;
    struct timezone tz;

    BSDgettimeofday(&tv, &tz);

    /* First get the seconds right */
    result = (unsigned) SECONDS_FROM_1900_to_1970 +
             (unsigned) tv.tv_sec -
             (unsigned) 60 * tz.tz_minuteswest +
             (unsigned) (tz.tz_dsttime ? 3600 : 0);

#if 0
    /* No timezone, no DST version ... */
    result = (unsigned) SECONDS_FROM_1900_to_1970 +
             (unsigned) tv.tv_sec;
#endif

    /* make seconds the high-order 32 bits */
    result = result << 32;

    /* Now get the fractional part. */
    usecOffset = (unsigned) tv.tv_usec * (unsigned) TWO_TO_THE_32_OVER_ONE_MILLION;
    /* printf("** %ld microsec is offset %x\n", tv.tv_usec, usecOffset); */

    result += usecOffset;

  /*    printf("* OSCTT_CurrentTime is %llx\n", result); */
    return result;
}

#else /* __sgi */

/* Instead of asking your operating system what time it is, it might be
   clever to find out the current time at the instant your application
   starts audio processing, and then keep track of the number of samples
   output to know how much time has passed. */

/* Loser version for systems that have no ability to tell the current time: */
OSCTimeTag OSCTT_CurrentTime(void) {
    return (OSCTimeTag) 1;
}

#endif /* __sgi */

#else /* Not HAS8BYTEINT */

#ifdef __linux

#include <math.h>
#include <sys/time.h>

#define SECONDS_FROM_1900_to_1970 2208988800UL /* 17 leap years */

/*
  Is this right: we do have nanosec precision,
  thus we convert usecs returned from gettimeofday()
  to nanosecs ?!!
*/
OSCTimeTag OSCTT_CurrentTime(void) {
  struct timeval tv;
  struct timezone tz;
    OSCTimeTag result;

  gettimeofday(&tv, &tz);

  /* tz.dsttime deprecated on Linux */
  result.seconds = tv.tv_sec - 60 * tz.tz_minuteswest + (uint4) SECONDS_FROM_1900_to_1970;
  result.fraction = (unsigned) tv.tv_usec * (unsigned) 1e3;
    return result;
}

OSCTimeTag OSCTT_PlusSeconds(OSCTimeTag tt, float secondsOffset) {
  double abs, frac;

  frac = modf(secondsOffset, &abs);

  tt.seconds  += abs;
  tt.fraction += frac * 1e9;

  return tt;
}

#else /* not __linux */

OSCTimeTag OSCTT_CurrentTime(void) {
    OSCTimeTag result;
  result.seconds = 0;
  result.fraction = 1;
    return result;
}

OSCTimeTag OSCTT_PlusSeconds(OSCTimeTag original, float secondsOffset) {
    OSCTimeTag result;
    result.seconds = 0;
    result.fraction = 1;
    return result;
}

#endif /* __linux */

OSCTimeTag OSCTT_BiggestPossibleTimeTag(void) {
    OSCTimeTag result;
  result.seconds = 0xffffffff;
  /* result.fraction = 0xffffffff; */
  result.fraction = 999999999;
  return result;
}

OSCTimeTag OSCTT_Immediately(void) {
  OSCTimeTag result;
    result.seconds = 0;
    result.fraction = 1;
    return result;
}

int OSCTT_Compare(OSCTimeTag left, OSCTimeTag right) {

  /*
      This puts the result of uint arithmetics
      into an int var.

    int highResult = left.seconds - right.seconds;

    if (highResult != 0) return highResult;

    return left.fraction - right.fraction;
  */

  if (left.seconds != right.seconds)
    return (left.seconds > right.seconds ? 1 : -1);
  else
    if (left.fraction != right.fraction)
      return (left.fraction > right.fraction ? 1 : -1);
    else
      return 0;
}

void OSCTT_SetFromInt(OSCTimeTag *self, unsigned int value) {
       if (self != (OSCTimeTag *) 0) { /* should be NULL */
               self->seconds = 0;
               self->fraction = value & 0xffffffff;
       }
}

#endif /* HAS8BYTEINT */

