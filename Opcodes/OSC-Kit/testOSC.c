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

/* testOSC.c

   test shell for this OSC package

   Matt Wright, 3/13/98

*/

#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"
#include "NetworkReturnAddress.h"
#include "OSC-receive.h"
#include "OSC-string-help.h"

#include <stdio.h>
#include <stdlib.h>

void SetUpAddrSpace(void);
void TestEmptyCase();
int   PretendToGetPacket();

OSCcontainer OSCTopLevelContainer;

void *MyInitTimeMalloc(int numBytes) {
    return malloc(numBytes);
}

void *MyRealTimeMalloc(int numBytes) {
    return 0;
}

int main() {
    OSCBoolean result;
    struct OSCAddressSpaceMemoryTuner t;
    struct OSCReceiveMemoryTuner rt;

    t.initNumContainers = 20;
    t.initNumMethods = 20;
    t.InitTimeMemoryAllocator = MyInitTimeMalloc;
    t.RealTimeMemoryAllocator = MyRealTimeMalloc;

    OSCTopLevelContainer = OSCInitAddressSpace(&t);

    rt.InitTimeMemoryAllocator = MyInitTimeMalloc;
    rt.RealTimeMemoryAllocator = MyRealTimeMalloc;
    rt.receiveBufferSize = 1000;
    rt.numReceiveBuffers = 10;
    rt.numQueuedObjects = 50;
    rt.numCallbackListNodes = 100;

    result = OSCInitReceive(&rt);

    if (result == FALSE) {
        printf("OSCInitReceive returned FALSE!\n");
        return;
    }

    SetUpAddrSpace();

    TestEmptyCase();

    PretendToGetPacket();

    printf("Test completed successfully!\n");
}

void BarCallback(void *context, int arglen, const void *args, OSCTimeTag when,
                 NetworkReturnAddressPtr returnAddr) {
    const int *intArgs = args;

    printf("Bar callback called!\n");
    printf("  Context %p, arglen %d, args %p, TT %llx, returnAddr %p\n",
           context, arglen, args, when, returnAddr);
    printf("  Args as ints: %d, %d\n", intArgs[0], intArgs[1]);
    printf("  Return address as string: \"%s\"\n", (char *) returnAddr);
    printf("\n");
}

void FarCallback(void *context, int arglen, const void *args, OSCTimeTag when,
                 NetworkReturnAddressPtr returnAddr) {
    const char *charArgs = args;
    char *error, *secondArg;

    printf("Far callback called!\n");
    printf("  Context %p, arglen %d, args %p, TT %llx, returnAddr %p\n",
           context, arglen, args, when, returnAddr);

    secondArg = OSCDataAfterAlignedString(charArgs, charArgs+arglen, &error);
    if (secondArg == 0) {
        printf("OSCDataAfterAlignedString error! %s\n", error);
    }

    printf("  Args as strings: %s, %s\n", charArgs, secondArg);
    printf("  Return address as string: \"%s\"\n", (char *) returnAddr);
    printf("\n");
}

void SetUpAddrSpace(void) {
    OSCcontainer foo, goo, hunkydory;
    char addr[100];
    struct OSCContainerQueryResponseInfoStruct cqinfo;
    struct OSCMethodQueryResponseInfoStruct QueryResponseInfo;

    printf("Getting address of top-level container\n");
    if (OSCGetAddressString(addr, 100, OSCTopLevelContainer) == FALSE) {
        printf("OSCGetAddressString returned FALSE!\n");
    }
    printf("It's \"%s\"\n", addr);

    printf("Printing whole address space before we put anything in it\n");
    OSCPrintWholeAddressSpace();

    OSCInitContainerQueryResponseInfo(&cqinfo);
    cqinfo.comment = "Foo!";

    if ((foo = OSCNewContainer("foo", OSCTopLevelContainer, &cqinfo)) == 0) {
        printf("OSCNewContainer returned FALSE!\n");
        return;
    }

    printf("Printing whole address space after we register /foo/\n");
    OSCPrintWholeAddressSpace();

    printf("Trying to get address of /foo/ in a 4 char array\n");
    if (OSCGetAddressString(addr, 4, foo) == FALSE) {
        printf("Good---OSCGetAddressString returned FALSE\n");
    } else {
        printf("OSCGetAddressString returned TRUE!!!\n");
    }

    printf("Trying to get address of /foo/ in a 5 char array\n");
    if (OSCGetAddressString(addr, 5, foo) == FALSE) {
        printf("Good---OSCGetAddressString returned FALSE\n");
    } else {
        printf("OSCGetAddressString returned TRUE!!!\n");
    }

    printf("Trying to get address of /foo/ in a 6 char array\n");
    if (OSCGetAddressString(addr, 6, foo) == FALSE) {
        printf("OSCGetAddressString returned FALSE!!!\n");
    } else {
        printf("Good---OSCGetAddressString returned TRUE.  Addr is %s\n", addr);
    }

    printf("Trying to get address of /foo/ in a 100 char array\n");
    if (OSCGetAddressString(addr, 100, foo) == FALSE) {
        printf("OSCGetAddressString returned FALSE!\n");
    }
    printf("It's \"%s\"\n", addr);

    OSCInitContainerQueryResponseInfo(&cqinfo);
    cqinfo.comment = "Everything's just Hunky-Dory!";

    if ((hunkydory = OSCNewContainer("hunkydory", foo, &cqinfo)) == 0) {
        printf("OSCNewContainer returned 0!\n");
        return;
    }

    printf("Trying to get address of /foo/hunkydory/ in a 100 char array\n");
    if (OSCGetAddressString(addr, 100, hunkydory) == FALSE) {
        printf("OSCGetAddressString returned FALSE!\n");
    }
    printf("It's \"%s\"\n", addr);

    OSCInitContainerQueryResponseInfo(&cqinfo);
    cqinfo.comment = "Goo is goopy.";

    if ((goo = OSCNewContainer("goo", OSCTopLevelContainer, &cqinfo)) == 0) {
        printf("OSCNewContainer returned 0!\n");
        return;
    }

    OSCInitMethodQueryResponseInfo(&QueryResponseInfo);
    QueryResponseInfo.description = "Get drunk in a bar";

    if (OSCNewMethod("bar", foo, BarCallback, (void *) 100, &QueryResponseInfo) == 0) {
        printf("OSCNewMethod returned 0!\n");
        return;
    }

    QueryResponseInfo.description = "Latvia is very far";

    if (OSCNewMethod("far", goo, FarCallback, (void *) 7, &QueryResponseInfo) == 0) {
        printf("OSCAddMethod returned 0!\n");
        return;
    }

    printf("Printing whole address space after we register everything\n");
    OSCPrintWholeAddressSpace();

    printf("Now register some aliases\n");

    if (OSCAddContainerAlias(goo, "slime") == FALSE) {
        printf("OSCAddContainerAlias returned FALSE!\n");
    }

    if (OSCAddContainerAlias(goo, "schmutz") == FALSE) {
        printf("OSCAddContainerAlias returned FALSE!\n");
    }

    if (OSCAddContainerAlias(goo, "spooge") == FALSE) {
        printf("OSCAddContainerAlias returned FALSE!\n");
    }

    if (OSCAddContainerAlias(goo, "glurpies") == FALSE) {
        printf("OSCAddContainerAlias returned FALSE!\n");
    }

    printf("Printing whole address space after registering aliases\n");
    OSCPrintWholeAddressSpace();

    printf("Finished registering the address space!\n\n\n");
}

void TestEmptyCase() {
    printf("Calling OSCInvokeMessagesThatAreReady, even though nothing's ready.\n");
#if 0
    OSCInvokeMessagesThatAreReady(2);
#endif
    OSCInvokeMessagesThatAreReady(OSCTT_PlusSeconds(OSCTT_CurrentTime(), 2));
}

static char ThePacket[] =
    "#bundle\0"
    "\0\0\0\0\3\0\0\0"
    "\0\0\0\x14"
    "/foo/bar\0\0\0\0"
    "\0\0\0\1\0\0\0\2"
    "\0\0\0\x14"
    "/goo/far\0\0\0\0"
    "a\0\0\0"
    "b\0\0\0";

int PretendToGetPacket() {
    OSCPacketBuffer p;
    char *buf;
    int *size;
    char *clientAddr;

    p = OSCAllocPacketBuffer();
    buf = OSCPacketBufferGetBuffer(p);
    size = OSCPacketBufferGetSize(p);
    clientAddr = (char *)OSCPacketBufferGetClientAddr(p);

    printf("Allocated a packet and got pointers to parts of it.\n"
           "Packet is %p, buffer is %p, size is %p, addr is %p\n",
           p, buf, size, clientAddr);

    memcpy(buf, ThePacket, sizeof(ThePacket)-1);
    (*size) = sizeof(ThePacket)-1;
    strcpy(clientAddr, "1750 Arch Street");

    printf("Calling OSCAcceptPacket()\n");
    OSCAcceptPacket(p);

    printf("Calling OSCBeProductiveWhileWaiting()\n");
    OSCBeProductiveWhileWaiting();

    printf("Calling OSCInvokeMessagesThatAreReady(0xffffffff)\n");
#if 0
    OSCInvokeMessagesThatAreReady(0xffffffff);
#endif
    OSCInvokeMessagesThatAreReady(OSCTT_BiggestPossibleTimeTag());

    return 0;
}
