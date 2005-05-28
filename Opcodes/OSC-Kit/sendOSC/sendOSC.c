/*
Copyright (c) 1996,1997.  The Regents of the University of California (Regents).
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

/* sendOSC.c

    Matt Wright, 6/3/97
    based on sendSC.c, which was based on a version by Adrian Freed

    Text-based OpenSoundControl client.  User can enter messages via command
    line arguments or standard input.

    Version 0.1: "play" feature
    Version 0.2: Message type tags.

*/

#define VERSION "http://cnmat.berkeley.edu/OpenSoundControl/sendOSC-0.1.html"

/*
compiling:
        cc -o sendOSC sendOSC.c htmsocket.c OpenSoundControl.c OSC_timeTag.c
*/

#include "OSC-client.h"
#include "htmsocket.h"

#include <stdio.h>
#include <stdlib.h>
/* #include <bstring.h> */
#include <string.h>

typedef struct {
    enum {INT, FLOAT, STRING} type;
    union {
        int i;
        float f;
        char *s;
    } datum;
} typedArg;

void CommandLineMode(int argc, char *argv[], void *htmsocket);
void InteractiveMode(void *htmsocket);
OSCTimeTag ParseTimeTag(char *s);
void ParseInteractiveLine(OSCbuf *buf, char *mesg);
typedArg ParseToken(char *token);
int WriteMessage(OSCbuf *buf, char *messageName, int numArgs, typedArg *args);
void SendBuffer(void *htmsocket, OSCbuf *buf);
void SendData(void *htmsocket, int size, char *data);
void fatal_error(char *s);
void complain(char *s, ...);

/* Exit status codes:
    0: successful
    2: Message(s) dropped because of buffer overflow
    3: Socket error
    4: Usage error
    5: Internal error
*/
static int exitStatus = 0;

static int useTypeTags = 1;

main(int argc, char *argv[]) {
    int portnumber;
    char *hostname = 0;
    void *htmsocket;

    argc--;
    argv++;

    if (argc == 0) {
        goto usageerror;
    }

    if (argc >= 1 && (strncmp(*argv, "-notypetags", 2) == 0)) {
        useTypeTags = 0;
        argv++;
        argc--;
    }

    if (argc >= 2 && (strncmp(*argv, "-r", 2) == 0)) {
        hostname = getenv("REMOTE_ADDR");
        if (hostname == NULL) {
            complain("sendSC -r: REMOTE_ADDR not in environment\n");
            exit(4);
        }
        argv++;
        argc--;
    }

    if (argc >= 3 && (strncmp(*argv, "-h", 2) == 0)) {
        hostname = argv[1];
        argv += 2;
        argc -= 2;
    }
    portnumber = atoi(*argv);
    argv++;
    argc--;

    htmsocket = OpenHTMSocket(hostname, portnumber);
    if (!htmsocket) {
        perror("Couldn't open socket: ");
        exit(3);
    }

    if (argc > 0) {
        printf("host %s, port %d, %s\n", hostname, portnumber,
               useTypeTags ? "use type tags" : "don't use type tags");
        CommandLineMode(argc, argv, htmsocket);
    } else {
        printf("sendOSC version " VERSION "\n");
        printf("by Matt Wright. Copyright (c) 1996, 1997 Regents of the University of California.\n");
        printf("host %s, port %d, %s\n", hostname, portnumber,
               useTypeTags ? "use type tags" : "don't use type tags");
        InteractiveMode(htmsocket);
    }
    CloseHTMSocket(htmsocket);
    exit(exitStatus);

    usageerror:
        complain("usage: %s [-notypetags] [-r] [-h target_host_name] port_number [message...]\n",
                 argv[-1]);
        exit(4);

}

#define MAX_ARGS 2000
#define SC_BUFFER_SIZE 32000
static char bufferForOSCbuf[SC_BUFFER_SIZE];

void CommandLineMode(int argc, char *argv[], void *htmsocket) {
    char *messageName;
    char *token;
    typedArg args[MAX_ARGS];
    int i,j, numArgs;
    OSCbuf buf[1];

    OSC_initBuffer(buf, SC_BUFFER_SIZE, bufferForOSCbuf);

    if (argc > 1) {
        if (OSC_openBundle(buf, OSCTT_Immediately())) {
            complain("Problem opening bundle: %s\n", OSC_errorMessage);
            return;
        }
    }

    for (i = 0; i < argc; i++) {
        messageName = strtok(argv[i], ",");
        if (messageName == NULL) {
            break;
        }

        j = 0;
        while ((token = strtok(NULL, ",")) != NULL) {
            args[j] = ParseToken(token);
            j++;
            if (j >= MAX_ARGS) {
                complain("Sorry; your message has more than MAX_ARGS (%d) arguments; ignoring the rest.\n",
                         MAX_ARGS);
                break;
            }
        }
        numArgs = j;

        WriteMessage(buf, messageName, numArgs, args);
    }

    if (argc > 1) {
        if (OSC_closeBundle(buf)) {
            complain("Problem closing bundle: %s\n", OSC_errorMessage);
            return;
        }
    }

    SendBuffer(htmsocket, buf);
}

#define MAXMESG 2048

void InteractiveMode(void *htmsocket) {
    char mesg[MAXMESG];
    OSCbuf buf[1];
    int bundleDepth = 0;    /* At first, we haven't seen "[". */

    OSC_initBuffer(buf, SC_BUFFER_SIZE, bufferForOSCbuf);

    while (fgets(mesg, MAXMESG, stdin) != NULL) {
        if (mesg[0] == '\n') {
          if (bundleDepth > 0) {
            /* Ignore blank lines inside a group. */
          } else {
            /* blank line => repeat previous send */
            SendBuffer(htmsocket, buf);
          }
          continue;
        }

        if (bundleDepth == 0) {
            OSC_resetBuffer(buf);
        }

        if (mesg[0] == '[') {
            OSCTimeTag tt = ParseTimeTag(mesg+1);
            if (OSC_openBundle(buf, tt)) {
                complain("Problem opening bundle: %s\n", OSC_errorMessage);
                OSC_resetBuffer(buf);
                bundleDepth = 0;
                continue;
            }
            bundleDepth++;
        } else if (mesg[0] == ']' && mesg[1] == '\n' && mesg[2] == '\0') {
            if (bundleDepth == 0) {
                complain("Unexpected ']': not currently in a bundle.\n");
            } else {
                if (OSC_closeBundle(buf)) {
                    complain("Problem closing bundle: %s\n", OSC_errorMessage);
                    OSC_resetBuffer(buf);
                    bundleDepth = 0;
                    continue;
                }

                bundleDepth--;
                if (bundleDepth == 0) {
                    SendBuffer(htmsocket, buf);
                }
            }
        } else {
            ParseInteractiveLine(buf, mesg);
            if (bundleDepth != 0) {
                /* Don't send anything until we close all bundles */
            } else {
                SendBuffer(htmsocket, buf);
            }
        }
    }
}

OSCTimeTag ParseTimeTag(char *s) {
    char *p, *newline;
    typedArg arg;

    p = s;
    while (isspace(*p)) p++;
    if (*p == '\0') return OSCTT_Immediately();

    if (*p == '+') {
        /* Time tag is for some time in the future.  It should be a
           number of seconds as an int or float */

        newline = strchr(s, '\n');
        if (newline != NULL) *newline = '\0';

        p++; /* Skip '+' */
        while (isspace(*p)) p++;

        arg = ParseToken(p);
        if (arg.type == STRING) {
            complain("warning: inscrutable time tag request: %s\n", s);
            return OSCTT_Immediately();
        } else if (arg.type == INT) {
            return OSCTT_PlusSeconds(OSCTT_CurrentTime(),
                                     (float) arg.datum.i);
        } else if (arg.type == FLOAT) {
            return OSCTT_PlusSeconds(OSCTT_CurrentTime(), arg.datum.f);
        } else {
            fatal_error("This can't happen!");
        }
    }

    if (isdigit(*p) || (*p >= 'a' && *p <='f') || (*p >= 'A' && *p <='F')) {
        /* They specified the 8-byte tag in hex */
        OSCTimeTag tt;
        if (sscanf(p, "%llx", &tt) != 1) {
            complain("warning: couldn't parse time tag %s\n", s);
            return OSCTT_Immediately();
        }
#ifndef HAS8BYTEINT
        if (ntohl(1) != 1) {
            /* tt is a struct of seconds and fractional part,
               and this machine is little-endian, so sscanf
               wrote each half of the time tag in the wrong half
               of the struct. */
            unsigned long temp; /* used to be undefined uint32 -- JPff */
            temp = tt.seconds;
            tt.seconds = tt.fraction ;
            tt.fraction = temp;
        }
#endif
        return tt;
    }

    complain("warning: invalid time tag: %s\n", s);
    return OSCTT_Immediately();
}

void ParseInteractiveLine(OSCbuf *buf, char *mesg) {
    char *messageName, *token, *p;
    typedArg args[MAX_ARGS];
    int thisArg;

    p = mesg;
    while (isspace(*p)) p++;
    if (*p == '\0') return;

    messageName = p;

    if (strcmp(messageName, "play\n") == 0) {
        /* Special kludge feature to save typing */
        typedArg arg;

        if (OSC_openBundle(buf, OSCTT_Immediately())) {
            complain("Problem opening bundle: %s\n", OSC_errorMessage);
            return;
        }

        arg.type = INT;
        arg.datum.i = 0;
        WriteMessage(buf, "/voices/0/tp/timbre_index", 1, &arg);

        arg.type = FLOAT;
        arg.datum.i = 0.0f;
        WriteMessage(buf, "/voices/0/tm/goto", 1, &arg);

        if (OSC_closeBundle(buf)) {
            complain("Problem closing bundle: %s\n", OSC_errorMessage);
        }

        return;
    }

    while (!isspace(*p) && *p != '\0') p++;
    if (isspace(*p)) {
        *p = '\0';
        p++;
    }

    thisArg = 0;
    while (*p != '\0') {
        /* flush leading whitespace */
        while (isspace(*p)) p++;
        if (*p == '\0') break;

        if (*p == '"') {
            /* A string argument: scan for close quotes */
            p++;
            args[thisArg].type = STRING;
            args[thisArg].datum.s = p;

            while (*p != '"') {
                if (*p == '\0') {
                    complain("Unterminated quote mark: ignoring line\n");
                    return;
                }
                p++;
            }
            *p = '\0';
            p++;
        } else {
            token = p;
            while (!isspace(*p) && (*p != '\0')) p++;
            if (isspace(*p)) {
                *p = '\0';
                p++;
            }
            args[thisArg] = ParseToken(token);
        }
        thisArg++;
        if (thisArg >= MAX_ARGS) {
          complain("Sorry, your message has more than MAX_ARGS (%d) arguments; ignoring the rest.\n",
                   MAX_ARGS);
          break;
        }
    }

    if (WriteMessage(buf, messageName, thisArg, args) != 0)  {
        complain("Problem sending message: %s\n", OSC_errorMessage);
    }
}

typedArg ParseToken(char *token) {
    char *p = token;
    typedArg returnVal;

    /* It might be an int, a float, or a string */

    if (*p == '-') p++;

    if (isdigit(*p) || *p == '.') {
        while (isdigit(*p)) p++;
        if (*p == '\0') {
            returnVal.type = INT;
            returnVal.datum.i = atoi(token);
            return returnVal;
        }
        if (*p == '.') {
            p++;
            while (isdigit(*p)) p++;
            if (*p == '\0') {
                returnVal.type = FLOAT;
                returnVal.datum.f = atof(token);
                return returnVal;
            }
        }
    }

    returnVal.type = STRING;
    returnVal.datum.s = token;
    return returnVal;
}

int WriteMessage(OSCbuf *buf, char *messageName, int numArgs, typedArg *args) {
    int j, returnVal;

    returnVal = 0;

#ifdef DEBUG
    printf("WriteMessage: %s ", messageName);

     for (j = 0; j < numArgs; j++) {
        switch (args[j].type) {
            case INT:
            printf("%d ", args[j].datum.i);
            break;

            case FLOAT:
            printf("%f ", args[j].datum.f);
            break;

            case STRING:
            printf("%s ", args[j].datum.s);
            break;

            default:
            fatal_error("Unrecognized arg type");
            exit(5);
        }
    }
    printf("\n");
#endif

    if (!useTypeTags) {
        returnVal = OSC_writeAddress(buf, messageName);
        if (returnVal) {
            complain("Problem writing address: %s\n", OSC_errorMessage);
        }
    } else {
        /* First figure out the type tags */
        char typeTags[MAX_ARGS+2];
        int i;

        typeTags[0] = ',';

        for (i = 0; i < numArgs; ++i) {
            switch (args[i].type) {
                case INT:
                typeTags[i+1] = 'i';
                break;

                case FLOAT:
                typeTags[i+1] = 'f';
                break;

                case STRING:
                typeTags[i+1] = 's';
                break;

                default:
                fatal_error("Unrecognized arg type");
                exit(5);
            }
        }
        typeTags[i+1] = '\0';

        returnVal = OSC_writeAddressAndTypes(buf, messageName, typeTags);
        if (returnVal) {
            complain("Problem writing address: %s\n", OSC_errorMessage);
        }
    }

     for (j = 0; j < numArgs; j++) {
        switch (args[j].type) {
            case INT:
            if ((returnVal = OSC_writeIntArg(buf, args[j].datum.i)) != 0) {
                return returnVal;
            }
            break;

            case FLOAT:
            if ((returnVal = OSC_writeFloatArg(buf, args[j].datum.f)) != 0) {
                return returnVal;
            }
            break;

            case STRING:
            if ((returnVal = OSC_writeStringArg(buf, args[j].datum.s)) != 0) {
                return returnVal;
            }
            break;

            default:
            fatal_error("Unrecognized arg type");
            exit(5);
        }
    }

    return returnVal;
}

void SendBuffer(void *htmsocket, OSCbuf *buf) {
#ifdef DEBUG
    printf("Sending buffer...\n");
#endif
    if (OSC_isBufferEmpty(buf)) return;
    if (!OSC_isBufferDone(buf)) {
        fatal_error("SendBuffer() called but buffer not ready!");
        exit(5);
    }
    SendData(htmsocket, OSC_packetSize(buf), OSC_getPacket(buf));
}

void SendData(void *htmsocket, int size, char *data) {
    if (!SendHTMSocket(htmsocket, size, data)) {
        perror("Couldn't send out socket: ");
        CloseHTMSocket(htmsocket);
        exit(3);
    }
}

void fatal_error(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(4);
}

#include <stdarg.h>
void complain(char *s, ...) {
    va_list ap;
    va_start(ap, s);
    vfprintf(stderr, s, ap);
    va_end(ap);
}

#ifdef COMPUTE_MESSAGE_SIZE
    /* Unused code to find the size of a message */

    /* Compute size */
    size = SynthControl_effectiveStringLength(messageName);

    for (j = 0; j < numArgs; j++) {
        switch (args[j].type) {
            case INT: case FLOAT:
            size += 4;
            break;

            case STRING:
            size += SynthControl_effectiveStringLength(args[j].datum.s);
            break;

            default:
            fatal_error("Unrecognized token type");
            exit(4);
        }
    }

    if (!SynthControl_willMessageFit(buf, size)) {
        complain("Message \"%s\" won't fit in buffer: dropping.", messageName);
        return;
    }
#endif
