/*
Copyright (c) 1992,1996.  The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Adrian Freed, The Center for New Music and Audio Technologies,
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
*/

 /* htmparam.h

	Adrian Freed
 	send parameters to htm servers by udp or UNIX protocol
 */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
typedef int bool;

/* open a socket for HTM communication to given  host on given portnumber */
/* if host is 0 then UNIX protocol is used (i.e. local communication) */
void *OpenHTMSocket(char *host, int portnumber);

/* send a buffer of data over htm socket, returns TRUE on success.
 Note that udp sends rarely fail. UNIX sends fail if a kernal buffer overflows */
bool SendHTMSocket(void *htmsendhandle, int length_in_bytes, void *buffer);

/* close the socket(2) and release memory associated with it */
void CloseHTMSocket(void *htmsendhandle);
