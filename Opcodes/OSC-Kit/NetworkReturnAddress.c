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
  NetworkReturnAddress.c

  This version implements UDP return addresses on SGI

  Matt Wright, 
  9/11/98
*/

#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"
#include "NetworkReturnAddress.h"


#include "NetworkUDP.h"



int SizeOfNetworkReturnAddress(void) {
    return sizeof(struct NetworkReturnAddressStruct);
}

#include <sys/types.h>
#include <sys/socket.h>

OSCBoolean NetworkSendReturnMessage(NetworkReturnAddressPtr addr,
				 int n,
				 void *buf) {
    if (addr == 0) return FALSE;

    return n == sendto(addr->sockfd, buf, n, 0,
                       (const struct sockaddr*)&(addr->cl_addr), addr->clilen);
}
