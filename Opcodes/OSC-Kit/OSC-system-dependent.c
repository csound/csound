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


/* OSC-system-dependent.c

   Matt Wright, 3/13/98

   File of procedures OSC has to call that are not part of the OSC package
   and that you, the developer adding OSC addressability to an application,
   must write in a way that makes sense in the context of your system.

   You should also look at OSC-timetag.c and see if there's a better way
   to handle time tags on your system.

*/

#include "OSC-common.h"

/* Printing stuff: for now, use stderr.  Some cleverer stuff we could do:

    - Make a silent mode where these don't do anything.
    - Return error messages via OSC to some client
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal_error(char *s, ...) {
    va_list ap;
    fprintf(stderr, "Fatal error: ");
    va_start(ap, s);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(-321);
}

void OSCProblem(char *s, ...) {
    va_list ap;
    fprintf(stderr, "OSC Problem: ");
    va_start(ap, s);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void OSCWarning(char *s, ...) {
    va_list ap;
    fprintf(stderr, "OSC Warning: ");
    va_start(ap, s);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
