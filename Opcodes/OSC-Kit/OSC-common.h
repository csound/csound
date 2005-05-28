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

/* OSC-common.h
   Simple stuff to #include everywhere in the OSC package

   by Matt Wright, 3/13/98
*/

/* Boolean type */

#if defined(TRUE)
#       undef TRUE              /* otherwise we will lose some typedefing */
#endif  /* defined(TRUE) */
#if defined(FALSE)
#       undef FALSE             /* same as above */
#endif /* defined(FALSE) */

typedef enum
{
    FALSE = 0,
    TRUE  = 1
} OSCBoolean;

/* Fixed byte width types */
typedef int int4;   /* 4 byte int */

/* Printing type procedures.  All take printf-style format string */

/* Catastrophic failure: print message and halt system */
void fatal_error(char *s, ...);

/* Error message for user */
void OSCProblem(char *s, ...);

/* Warning for user */
void OSCWarning(char *s, ...);

