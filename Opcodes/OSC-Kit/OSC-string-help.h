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

/* OSC-string-help.h
   Procedures that could be useful to programmers writing OSC methods that
   take string arguments.

   by Matt Wright, 3/19/98
*/

/* Use this to deal with OSC null-padded 4 byte-aligned strings

   The argument is a block of data beginning with a string.  The string
   has (presumably) been padded with extra null characters so that the
   overall length is a multiple of 4 bytes.  Return a pointer to the next
   byte after the null byte(s).  The boundary argument points to the
   character after the last valid character in the buffer---if the string
   hasn't ended by there, something's wrong.

   If the data looks wrong, return 0, and set *errorMsg */

char *OSCDataAfterAlignedString(const char *string, const char *boundary, char **errorMsg);


/* Given a normal C-style string with a single padding byte, return the
   length of the string including the necessary 1-4 padding bytes.
   (Basically strlen()+1 rounded up to the next multiple of 4.) */
int OSCPaddedStrlen(const char *s);

/* Copy a given C-style string into the given destination, including the
   requisite padding byte(s).  Unlike strcpy(), this returns a pointer to
   the next character after the copied string's null bytes, like
   what OSCDataAfterAlignedString() returns. */
char *OSCPaddedStrcpy(char *target, const char *source);


/* Given an args pointer that should be nothing but a list of strings, fill
   result[] with pointers to the beginnings of each string, and set
   *numStrings to be the number of strings found.  maxStrings gives the size
   of the result array.  Return FALSE if any strings are malformatted or if
   there are more than maxStrings many strings. */

OSCBoolean OSCParseStringList(const char *result[], int *numStrings, int maxStrings,
			   const char *args, int numBytes);
