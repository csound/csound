/*
Copyright (c) 1996.  The Regents of the University of California (Regents).
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
*/


/* 
  Author: Matt Wright
  Version 2.1
 */


/* Here are the possible values of the state field: */

#define EMPTY 0	       /* Nothing written to packet yet */
#define ONE_MSG_ARGS 1 /* Packet has a single message; gathering arguments */
#define NEED_COUNT 2   /* Just opened a bundle; must write message name or
			  open another bundle */
#define GET_ARGS 3     /* Getting arguments to a message.  If we see a message
			  name or a bundle open/close then the current message
			  will end. */
#define DONE 4         /* All open bundles have been closed, so can't write 
		          anything else */

#include "OSC-client.h"

char *OSC_errorMessage;


static int strlen(char *s);
static int OSC_padString(char *dest, char *str);

void OSC_initBuffer(OSCbuf *buf, int size, char *byteArray) {
    buf->buffer = byteArray;
    buf->size = size;
    OSC_resetBuffer(buf);
}

void OSC_resetBuffer(OSCbuf *buf) {	
    buf->bufptr = buf->buffer;
    buf->state = EMPTY;
    buf->bundleDepth = 0;
    buf->prevCounts[0] = 0;
}

int OSC_isBufferEmpty(OSCbuf *buf) {
    return buf->bufptr == buf->buffer;
}

int OSC_freeSpaceInBuffer(OSCbuf *buf) {
    return buf->size - (buf->bufptr - buf->buffer);
}

int OSC_isBufferDone(OSCbuf *buf) {
    return (buf->state == DONE || buf->state == ONE_MSG_ARGS);
}

char *OSC_getPacket(OSCbuf *buf) {
#ifdef ERROR_CHECK_GETPACKET
    if (buf->state == DONE || buf->state == ONE_MSG_ARGS) {
	return buf->buffer;
    } else {
	OSC_errorMessage = "Packet has unterminated bundles";
	return 0;
    }
#else
    return buf->buffer;
#endif
}

int OSC_packetSize(OSCbuf *buf) {
#ifdef ERROR_CHECK_PACKETSIZE
    if (buf->state == DONE || buf->state == ONE_MSG_ARGS) {
	return (buf->bufptr - buf->buffer);
    } else {
        OSC_errorMessage = "Packet has unterminated bundles";
        return 0;
    }
#else
    return (buf->bufptr - buf->buffer);
#endif
}

#define CheckOverflow(buf, bytesNeeded) { \
    if ((bytesNeeded) > OSC_freeSpaceInBuffer(buf)) { \
	OSC_errorMessage = "buffer overflow"; \
	return 1; \
    } \
}

static void PatchMessageSize(OSCbuf *buf) {
    int4byte size;
    size = buf->bufptr - ((char *) buf->thisMsgSize) - 4;
    *(buf->thisMsgSize) = size;
}

int OSC_openBundle(OSCbuf *buf, OSCTimeTag tt) {
    if (buf->state == ONE_MSG_ARGS) {
	OSC_errorMessage = "Can't open a bundle in a one-message packet";
	return 3;
    }

    if (buf->state == DONE) {
	OSC_errorMessage = "This packet is finished; can't open a new bundle";
	return 4;
    }

    if (++(buf->bundleDepth) >= MAX_BUNDLE_NESTING) {
	OSC_errorMessage = "Bundles nested too deeply; change MAX_BUNDLE_NESTING in OpenSoundControl.h";
	return 2;
    }

    if (buf->state == GET_ARGS) {
	PatchMessageSize(buf);
    }

    if (buf->state == EMPTY) {
	/* Need 16 bytes for "#bundle" and time tag */
	CheckOverflow(buf, 16);
    } else {
	/* This bundle is inside another bundle, so we need to leave
	   a blank size count for the size of this current bundle. */
	CheckOverflow(buf, 20);
	*((int4byte *)buf->bufptr) = 0xaaaaaaaa;
        buf->prevCounts[buf->bundleDepth] = (int4byte *)buf->bufptr;

	buf->bufptr += 4;
    }

    buf->bufptr += OSC_padString(buf->bufptr, "#bundle");
    *((OSCTimeTag *) buf->bufptr) = tt;
    buf->bufptr += sizeof(OSCTimeTag);

    buf->state = NEED_COUNT;
    return 0;
}


int OSC_closeBundle(OSCbuf *buf) {
    if (buf->bundleDepth == 0) {
	/* This handles EMPTY, ONE_MSG, ARGS, and DONE */
	OSC_errorMessage = "Can't close bundle; no bundle is open!";
	return 5;
    }

    if (buf->state == GET_ARGS) {
        PatchMessageSize(buf);
    }

    if (buf->bundleDepth == 1) {
	/* Closing the last bundle: No bundle size to patch */
	buf->state = DONE;
    } else {
	/* Closing a sub-bundle: patch bundle size */
	int size = buf->bufptr - ((char *) buf->prevCounts[buf->bundleDepth]) - 4;
	*(buf->prevCounts[buf->bundleDepth]) = size;
	buf->state = NEED_COUNT;
    }

    --buf->bundleDepth;
    return 0;
}
	

int OSC_closeAllBundles(OSCbuf *buf) {
    if (buf->bundleDepth == 0) {
        /* This handles EMPTY, ONE_MSG, ARGS, and DONE */
        OSC_errorMessage = "Can't close all bundles; no bundle is open!";
        return 6;
    }

    while (buf->bundleDepth > 0) {
	OSC_closeBundle(buf);
    }
    return 0;
}

int OSC_writeAddress(OSCbuf *buf, char *name) {
    int4byte paddedLength;

    if (buf->state == ONE_MSG_ARGS) {
	OSC_errorMessage = "This packet is not a bundle, so you can't write another address";
	return 7;
    }

    if (buf->state == DONE) {
        OSC_errorMessage = "This packet is finished; can't write another address";
        return 8;
    }

    paddedLength = OSC_effectiveStringLength(name);

    if (buf->state == EMPTY) {
	/* This will be a one-message packet, so no sizes to worry about */
	CheckOverflow(buf, paddedLength);
	buf->state = ONE_MSG_ARGS;
    } else {
	/* GET_ARGS or NEED_COUNT */
	CheckOverflow(buf, 4+paddedLength);
	if (buf->state == GET_ARGS) {
	    /* Close the old message */
	    PatchMessageSize(buf);
	}
	buf->thisMsgSize = (int4byte *)buf->bufptr;
	*(buf->thisMsgSize) = 0xbbbbbbbb;
	buf->bufptr += 4;
	buf->state = GET_ARGS;
    }

    /* Now write the name */
    buf->bufptr += OSC_padString(buf->bufptr, name);
    return 0;
}

int OSC_writeFloatArg(OSCbuf *buf, float arg) {
    CheckOverflow(buf, 4);
    *((float *) buf->bufptr) = arg;
    buf->bufptr += 4;
    return 0;
}

int OSC_writeFloatArgs(OSCbuf *buf, int numFloats, float *args) {
    int i;
    CheckOverflow(buf, 4 * numFloats);
    for (i = 0; i < numFloats; i++) {
	*((float *) buf->bufptr) = args[i];
	buf->bufptr += 4;
    }
    return 0;
}

int OSC_writeIntArg(OSCbuf *buf, int4byte arg) {
    CheckOverflow(buf, 4);
    *((int4byte *) buf->bufptr) = arg;
    buf->bufptr += 4;
    return 0;
}

int OSC_writeStringArg(OSCbuf *buf, char *arg) {
    CheckOverflow(buf, OSC_effectiveStringLength(arg));
    buf->bufptr += OSC_padString(buf->bufptr, arg);
    return 0;
}

/* String utilities */

static int strlen(char *s) {
    int i;
    for (i=0; s[i] != '\0'; i++) /* Do nothing */ ;
    return i;
}

#define STRING_ALIGN_PAD 4
int OSC_effectiveStringLength(char *string) {
    int len = strlen(string) + 1;  /* We need space for the null char. */
    
    /* Round up len to next multiple of STRING_ALIGN_PAD to account for alignment padding */
    if ((len % STRING_ALIGN_PAD) != 0) {
        len += STRING_ALIGN_PAD - (len % STRING_ALIGN_PAD);
    }
    return len;
}

static int OSC_padString(char *dest, char *str) {
    int i;
    
    for (i = 0; str[i] != '\0'; i++) {
        dest[i] = str[i];
    }
    
    dest[i] = '\0';
    i++;
    
    for (; (i % STRING_ALIGN_PAD) != 0; i++) {
        dest[i] = '\0';
    }
    
    return i;
}
 
