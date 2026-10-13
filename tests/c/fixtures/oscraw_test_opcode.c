#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
extern int csound_test_oscraw_recvfrom(SOCKET, char *, int, int,
                                       struct sockaddr *, int *);
#else
#include <sys/types.h>
#include <sys/socket.h>
extern ssize_t csound_test_oscraw_recvfrom(int, void *, size_t, int,
                                           struct sockaddr *, socklen_t *);
#endif
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define recvfrom csound_test_oscraw_recvfrom
#include "../../../Opcodes/sockrecv.c"

OENTRY *csound_test_oscraw_opcode(void)
{
    return &sockrecv_localops[6];
}
#endif
