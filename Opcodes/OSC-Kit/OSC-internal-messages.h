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

/* OSC-internal-messages.h

   Interface for having an application send OSC messages to itself
   internally.

   All these procedures return FALSE if unable to deliver the message.

   Matt Wright, 3/17/98

*/

/* Send a message immediately, with no return address.  This procedure
   returns after the message has been sent (or has failed to be sent),
   so the memory for address and args can be on the stack.  Returns FALSE
   if there's a problem; TRUE otherwise. */
OSCBoolean OSCSendInternalMessage(char *address, int arglen, void *args);


/* Same thing, but with a return address supplied. */
OSCBoolean OSCSendInternalMessageWithRSVP(char *address, int arglen,  void *args, 
				       NetworkReturnAddressPtr returnAddr);


/* Schedule some messages to occur at a given time.  This allocates one of the
   OSCPacketBuffer structures (see OSC-receive.h) to hold the addresses and argument
   data until the messages take effect, so if you're going to call this, you
   should take this use of packets into account in setting the
   numReceiveBuffers argument to OSCInitReceive().

   This provides an less general interface than OSC's bundle mechanism, because
   the bundle of messages you provide cannot include subbundles. 

   The addresses, arglens, and args arguments are arrays of size numMessages.

   There's no return address argument because you're not allowed to save a network
   return address for later use.
*/

OSCBoolean OSCScheduleInternalMessages(OSCTimeTag when, int numMessages, 
				    char **addresses, int *arglens,
				    void **args);
