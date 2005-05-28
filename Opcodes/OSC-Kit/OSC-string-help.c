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

/* OSC-string-help.c
   Procedures that could be useful to programmers writing OSC methods that
   take string arguments.

   by Matt Wright, 3/19/98
*/

#include "OSC-common.h"  /* For OSCBoolean */

#define STRING_ALIGN_PAD 4

char *OSCDataAfterAlignedString(const char *string, const char *boundary, char **errorMsg) {

    int i;

    if ((boundary - string) %4 != 0) {
        fatal_error("DataAfterAlignedString: bad boundary\n");
    }

    for (i = 0; string[i] != '\0'; i++) {
        if (string + i >= boundary) {
            (*errorMsg) = "DataAfterAlignedString: Unreasonably long string";
            return 0;
        }
    }

    /* Now string[i] is the first null character */
    i++;

    for (; (i % STRING_ALIGN_PAD) != 0; i++) {
        if (string + i >= boundary) {
            (*errorMsg) = "Unreasonably long string";
            return 0;
        }
        if (string[i] != '\0') {
            (*errorMsg) = "Incorrectly padded string.";
            return 0;
        }
    }

    return (char *) (string+i);
}

int OSCPaddedStrlen(const char *s) {
    int i;

    for (i = 0; *s != '\0'; s++, i++) {
        /* do nothing */
    }

    /* Now i is the length with no null bytes.  We need 1-4 null bytes,
       to make the total length a multiple of 4.   So we add 4, as if
       we need 4 null bytes, then & 0xfffffffc to round down to the nearest
       multiple of 4. */

    return (i + 4) & 0xfffffffc;
}

char *OSCPaddedStrcpy(char *target, const char *source)
 {
    while ((*target++ = *source++)) {
        /* do nothing */
    }

    /* That copied one null byte */
    while (((int) target) % 4 != 0) {
        *target = '\0';
        target++;
    }
    return target;
}

OSCBoolean OSCParseStringList(const char *result[], int *numStrings, int maxStrings,
                           const char *args, int numBytes) {
    int numFound;
    const char *p;
    const char *boundary = args + numBytes;
    char *errorMessage;

    p = args;

    for (numFound = 0; numFound < maxStrings; ++numFound) {
        if (p == boundary) {
            *numStrings = numFound;
            return TRUE;
        }

        result[numFound] = p;
        p = OSCDataAfterAlignedString(p, boundary, &errorMessage);
        if (p == 0) return FALSE;
    }
    return FALSE;
}

