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

udp-simplesynth.c

   Trivial two-sine-wave synthesizer controlled by OpenSound Control
   via UDP.  Written for SGI/IRIX.

   Based on Adrian Freed's "simplesynth.c"

   by Matt Wright, 9/22/98

*/

#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"
#include "OSC-receive.h"

/* for select(2) */
#include <unistd.h>
#include <sys/types.h>
#if !defined(HAS_NO_BSTRING_H)
#       include <bstring.h>
#endif /* defined(HAS_NO_BSTRING_H) */
#include <sys/time.h>

#include <audio.h>         /* SGI audio library */
#if !defined(LINUX)
#include <sys/schedctl.h>  /* for scheduler control */
#include <sys/lock.h>      /* for memory lock */
#endif /* !defined(LINUX) */

#include <math.h>
#define PI 3.14159265358979323

/* For UDP stuff */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <grp.h>
#include <sys/file.h>
#include <sys/prctl.h>

#define SRATE 44100.0 /* Hz */
#define OUTPUTQUEUESIZE 1024 /* Samples */

#define GAIN 0.1

typedef int FileDescriptor;

static FileDescriptor initudp(int port) {
        struct sockaddr_in serv_addr;
        int n, sockfd;

        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                        return sockfd;
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);

        if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
                perror("unable to  bind\n");
                return -1;
        }

        fcntl(sockfd, F_SETFL, FNDELAY);
        return sockfd;
}

static void closeudp(int sockfd) {
    close(sockfd);
}

static int time_to_quit;

static void catch_sigint()  {
   time_to_quit = 1;
}

#include "NetworkUDP.h"

void ReceivePacket(int sockfd) {
    OSCPacketBuffer pb;
    struct NetworkReturnAddressStruct *ra;
    int maxclilen=sizeof(struct sockaddr_in);
    int n;
    int capacity = OSCGetReceiveBufferSize();
    int morePackets = 1;
    char *buf;

    while(morePackets) {
        pb = OSCAllocPacketBuffer();
        if (!pb) {
            OSCWarning("Out of memory for packet buffers---had to drop a packet!");
            /* How do I actually gobble down the packet? */
            return;
        }
        buf = OSCPacketBufferGetBuffer(pb);

        /* The type NetworkReturnAddressPtr is const in two ways, so that
           callback procedures that are passed the pointer can neither change
           the pointer itself or the data the pointer points to.  But here's
           where we fill in the return address, so we do a cast to a non-const
           type. */

        ra = (struct NetworkReturnAddressStruct *) OSCPacketBufferGetClientAddr(pb);
        ra->clilen = maxclilen;
        ra->sockfd = sockfd;

/*      printf("* recvfrom(sockfd %d, buf %p, capacity %d)\n", sockfd, buf, capacity); */
        n = recvfrom(sockfd, buf, capacity, 0, &(ra->cl_addr), &(ra->clilen));

        if (n > 0) {
            int *sizep = OSCPacketBufferGetSize(pb);
            *sizep = n;
            OSCAcceptPacket(pb);
            if (time_to_quit) return;
        } else {
            OSCFreePacket(pb);
            morePackets = 0;
        }
    }
}

/*
 * linux has different system calls for scheduling priority handling
 */
#if !defined(LINUX)
void InitPriority(void) {
    /* set process priority high */
    if (schedctl (NDPRI,getpid(), NDPHIMIN) < 0)
            perror ("schedctl NDPNORMMIN");

    /* lock memory to avoid paging */
    plock(PROCLOCK);

    /* schedctl requires set user id root, so we need to return to regular
       user id to avoid security problems */
    setuid(getuid());
}
#else /* defined(LINUX) */
void InitPriority(void) {
    /* empty until someone has some time to work on it... */
}
#endif /* !defined(LINUX) */

float the_sample_rate;

FileDescriptor InitAudio(ALport *alpp) {
        /* initialize audio driver
           for 16-bit 44100kHz monophonic sample source to DACs */
        ALconfig alc;
        alc = ALnewconfig();
        ALsetwidth (alc, AL_SAMPLE_16);
        ALsetqueuesize (alc, OUTPUTQUEUESIZE);
        ALsetchannels(alc, (long)1);
        *alpp = ALopenport("obuf", "w", alc);
        {
                long pvbuf[2];
                long pvlen=2;

                pvbuf[0] = AL_OUTPUT_RATE;
                pvbuf[1] = AL_RATE_44100;
                ALsetparams(AL_DEFAULT_DEVICE, pvbuf, pvlen);
                the_sample_rate = 44100.0f;
        }

        /* obtain a file descriptor associated with sound output port */
        return ALgetfd(*alpp);
}

typedef struct {
    double f;   /* sine wave frequency */
    float g;    /* gain */
    double t;   /* time */
} SynthState;

void InitVoice(SynthState *s, float freq) {
    s->f = freq;
    s->g = 0.1;
    s->t = 0.0;
}

#define VSIZE 32 /* vector size */

void Synthesize(ALport alp, SynthState *v1, SynthState *v2) {
   short samplebuffer[VSIZE];
    int i;
    for (i = 0; i < VSIZE; ++i) {
        /* appropriately scaled sine wave */
        samplebuffer[i] = 32767.0f * GAIN *
            (v1->g * sin(2.0 * PI * v1->f * v1->t) +
             v2->g * sin(2.0 * PI * v2->f * v2->t));
        v1->t += 1.0 / SRATE; /* the march of time */
        v2->t += 1.0 / SRATE; /* the march of time */
    }
    /* send samples out the door */
    ALwritesamps(alp, samplebuffer, VSIZE);
}

#define BIGGER_OF(a,b) ((a)>(b)?(a):(b))

void MainLoop(ALport alp, FileDescriptor dacfd, FileDescriptor sockfd, SynthState *v1, SynthState *v2) {
/*    int hwm = 300, lwm = 256; */
    int hwm = 1000, lwm = 800;
    fd_set read_fds, write_fds;

    /* largest file descriptor to search for */
    int nfds = BIGGER_OF(dacfd, sockfd) + 1;

    printf("MainLoop: dacfd %d, sockfd %d, nfds %d\n", dacfd, sockfd, nfds);

    time_to_quit = 0;
    sigset(SIGINT, catch_sigint);       /* set sig handler       */

    while(!time_to_quit) {

        /* compute sine wave samples while the sound output buffer is below
           the high water mark */

        while (ALgetfilled(alp) < hwm) {
            Synthesize(alp, v1, v2);
        }

        /* Figure out the time tag corresponding to the time in the future that we haven't
           computed any samples for yet. */
        OSCInvokeAllMessagesThatAreReady(OSCTT_PlusSeconds(OSCTT_CurrentTime(),
                                                           ALgetfilled(alp) / the_sample_rate));

#if !defined(LINUX)/* ALsetfillpoint is not defined in the libaudio I have */
        /* set the low water mark, i.e. when we want control from select(2) */
        ALsetfillpoint(alp, OUTPUTQUEUESIZE - lwm);
#endif /* !defined(LINUX) */

        /* set up select */
        FD_ZERO(&read_fds);     /* clear read_fds */
        FD_ZERO(&write_fds);    /* clear write_fds */
        FD_SET(dacfd, &write_fds);
        FD_SET(sockfd, &read_fds);

        FD_SET(0, &read_fds);   /* stdin */

        /* give control back to OS scheduler to put us to sleep until the DAC
           queue drains and/or a character is available from standard input */

        if (select(nfds, &read_fds, &write_fds, (fd_set * )0, (struct timeval *)0) < 0) {
            /* select reported an error */
            perror("bad select");
            goto quit;
        }

        if(FD_ISSET(sockfd, &read_fds)) {
            ReceivePacket(sockfd);
        }

        /* is there a character in the queue? */
        if (FD_ISSET(0, &read_fds)) {
            /* this will never block */
            char c = getchar();

            if (c == 'q') {
                /* quit */
                break;
            } else if ((c <= '9') && (c >= '0')) {
                /* tweak frequency */
                v1->f = 440.0 + 100.0 * (c - '0');
            }
        }
    }
quit:
    ALcloseport(alp);
    closeudp(sockfd);
}

/* OSC stuff */

void *MyInitTimeMalloc(int numBytes) {
    void *result = malloc(numBytes);
/*    printf("** MyInitTimeMalloc(%6x bytes): %p -> %p\n", numBytes, result, ((char *)result)+numBytes); */
    return result;
}

void *MyRealTimeMalloc(int numBytes) {
    return 0;
}

void FreqMethod(void *vs, int arglen, const void *vargs, OSCTimeTag when, NetworkReturnAddressPtr ra) {
    SynthState *s = vs;
    const float *args = vargs;

    if (arglen == 4) {
/*      printf("*** FreqMethod arg %f, when %llx\n", args[0], when); */
        s->f = args[0];
    } else {
        OSCWarning("Wrong arglen to FreqMethod: %d", arglen);
    }
}

void ScaleMethod(void *vs, int arglen, const void *vargs, OSCTimeTag when, NetworkReturnAddressPtr ra) {
    int i;
    OSCTimeTag now = OSCTT_CurrentTime();
    OSCBoolean r;

    float newarg[1];
    float *ptrToArgs[1];
    char *address = "/sine1/f";
    int newarglen = 4;

/*    printf("*ScaleMethod (context %p, arglen %d, args %p, when %llx, ra %p)\n",
           vs, arglen, vargs, when, ra); */

    ptrToArgs[0] = newarg;
    for (i = 0; i < 9; ++i) {
        newarg[0] = 100.f + i * 50.f;

        r = OSCScheduleInternalMessages(OSCTT_PlusSeconds(now, (float) i), 1, &address, &newarglen, ptrToArgs);

        if (r == FALSE) {
            OSCWarning("ScaleMethod: OSCScheduleInternalMessages returned FALSE");
        }
    }

}

void GainMethod(void *vs, int arglen, const void *vargs, OSCTimeTag when, NetworkReturnAddressPtr ra) {
    SynthState *s = vs;
    const float *args = vargs;

    if (arglen == 4) {
        s->g = args[0];
    } else {
        OSCWarning("Wrong arglen to GainMethod: %d", arglen);
    }
}

void QuitMethod(void *dummy, int arglen, const void *args, OSCTimeTag when, NetworkReturnAddressPtr ra) {
    time_to_quit = 1;
}

void EchoMethod(void *dummy, int arglen, const void *args, OSCTimeTag when, NetworkReturnAddressPtr ra) {
    NetworkSendReturnMessage(ra, arglen, args);
}

#define MAX_NUM_STRINGS 10
void AllMyStringsMethod(void *dummy, int arglen, const void *args, OSCTimeTag when, NetworkReturnAddressPtr ra) {
    char *strings[MAX_NUM_STRINGS];
    int numFound;
    OSCBoolean r;
    int i;

    r = OSCParseStringList(strings, &numFound, MAX_NUM_STRINGS, args, arglen);

    if (!r) {
        OSCWarning("AllMyStringsMethod: OSCParseStringList() returned FALSE");
        return;
    }

    printf("\nAll My Strings:\n");
    for (i = 0; i < numFound; ++i) {
        printf(" strings[%d] is %p: \"%s\"\n", i, strings[i], strings[i]);
    }
}

void InitOSCAddrSpace(SynthState *v1, SynthState *v2) {
    OSCBoolean result;
    OSCcontainer OSCTopLevelContainer, sine1, sine2;
    struct OSCAddressSpaceMemoryTuner t;
    struct OSCContainerQueryResponseInfoStruct cqinfo;
    struct OSCMethodQueryResponseInfoStruct QueryResponseInfo;

    /* Address space */

    t.initNumContainers = 20;
    t.initNumMethods = 20;
    t.InitTimeMemoryAllocator = MyInitTimeMalloc;
    t.RealTimeMemoryAllocator = MyRealTimeMalloc;

    OSCTopLevelContainer = OSCInitAddressSpace(&t);

    OSCInitContainerQueryResponseInfo(&cqinfo);
    sine1 = OSCNewContainer("sine1", OSCTopLevelContainer, &cqinfo);
    sine2 = OSCNewContainer("sine2", OSCTopLevelContainer, &cqinfo);

    OSCInitMethodQueryResponseInfo(&QueryResponseInfo);
    OSCNewMethod("f", sine1, FreqMethod, v1, &QueryResponseInfo);
    OSCNewMethod("f", sine2, FreqMethod, v2, &QueryResponseInfo);
    OSCNewMethod("a", sine1, GainMethod, v1, &QueryResponseInfo);
    OSCNewMethod("a", sine2, GainMethod, v2, &QueryResponseInfo);
    OSCNewMethod("g", sine1, GainMethod, v1, &QueryResponseInfo);
    OSCNewMethod("g", sine2, GainMethod, v2, &QueryResponseInfo);
    OSCNewMethod("quit", OSCTopLevelContainer, QuitMethod, 0, &QueryResponseInfo);
    OSCNewMethod("echo", OSCTopLevelContainer, EchoMethod, 0, &QueryResponseInfo);
    OSCNewMethod("scale", OSCTopLevelContainer, ScaleMethod, 0, &QueryResponseInfo);
    OSCNewMethod("allmystrings", OSCTopLevelContainer, AllMyStringsMethod, 0, &QueryResponseInfo);
}

void InitOSCReceive() {
    struct OSCReceiveMemoryTuner rt;
    OSCBoolean result;

    rt.InitTimeMemoryAllocator = MyInitTimeMalloc;
    rt.RealTimeMemoryAllocator = MyRealTimeMalloc;
    rt.receiveBufferSize = 1000;
    rt.numReceiveBuffers = 100;
    rt.numQueuedObjects = 200;
    rt.numCallbackListNodes = 100;

    result = OSCInitReceive(&rt);

    if (result == FALSE) {
        fatal_error("OSCInitReceive returned FALSE!\n");
    }

}

void InitOSC(SynthState *v1, SynthState *v2) {
    InitOSCAddrSpace(v1, v2);
    InitOSCReceive();
}

int main() {
    ALport alp;
    FileDescriptor dacfd, sockfd;
    int udp_port = 7000;
    SynthState voices[2];

    InitVoice(voices, 440);
    InitVoice(voices+1, 220);

    dacfd = InitAudio(&alp);
    sockfd = initudp(udp_port);
    if(sockfd<0) {
        perror("initudp");
        return;
    }

    InitPriority();

    InitOSC(voices, voices+1);

    MainLoop(alp, dacfd, sockfd, voices, voices+1);

    return 0;
}
