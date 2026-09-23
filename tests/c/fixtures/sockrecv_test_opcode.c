#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
static int receive_packet(SOCKET, char *, int, int, struct sockaddr *, int *);
#else
#include <sys/socket.h>
static ssize_t receive_packet(int, void *, size_t, int, struct sockaddr *, socklen_t *);
#endif
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define recvfrom receive_packet
#include "../../../Opcodes/sockrecv.c"
#undef recvfrom

static SOCKRECV *receiver;
static const MYFLT *packet;
static size_t packet_size;

#if defined(WIN32) && !defined(__CYGWIN__)
static int receive_packet(SOCKET socket, char *out, int size, int flags,
                          struct sockaddr *from, int *from_size)
#else
static ssize_t receive_packet(int socket, void *out, size_t size, int flags,
                              struct sockaddr *from, socklen_t *from_size)
#endif
{
    memcpy(out, packet, packet_size);
    receiver->threadon = 0;
    return (int)packet_size;
}

void *csound_test_sockrecv_create(CSOUND *cs, INSDS *instance,
                                 MYFLT *left, MYFLT *right, int32_t samples)
{
    SOCKRECV *p = calloc(1, sizeof(SOCKRECV));
    p->cs = cs;
    p->h.insdshead = instance;
    p->ptr1 = left;
    p->ptr2 = right;
    p->channels = right == NULL ? 1 : 2;
    p->buffsize = MTU / sizeof(MYFLT);
    p->buf = calloc(1, MTU);
    p->tmp.auxp = calloc(1, MTU);
    p->cb = cs->CreateCircularBuffer(cs, samples / p->channels,
                                    p->channels * sizeof(MYFLT));
    return p;
}

void csound_test_sockrecv_packet(void *state, const MYFLT *data, int32_t samples)
{
    receiver = state;
    packet = data;
    packet_size = samples * sizeof(MYFLT);
    receiver->threadon = 1;
    udpRecv(receiver);
}

int32_t csound_test_sockrecv_perform(void *state, int32_t control)
{
    SOCKRECV *p = state;
    return control ? send_recv_k(p->cs, p) :
           p->channels == 2 ? send_recvS(p->cs, p) : send_recv(p->cs, p);
}

void csound_test_sockrecv_destroy(void *state)
{
    SOCKRECV *p = state;
    p->cs->DestroyCircularBuffer(p->cs, p->cb);
    free(p->buf);
    free(p->tmp.auxp);
    free(p);
}
#endif
