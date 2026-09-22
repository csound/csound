#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
typedef SOCKET TestSocket;
#else
#include <sys/socket.h>
typedef int TestSocket;
#endif
extern TestSocket csound_test_strecv_accept(TestSocket, struct sockaddr *,
                                           socklen_t *);
extern int csound_test_strecv_recv(TestSocket, void *, size_t, int);
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define accept csound_test_strecv_accept
#define recv csound_test_strecv_recv
#include "../../../Opcodes/sockrecv.c"

OENTRY *csound_test_strecv_opcode(void)
{
    return &sockrecv_localops[4];
}
#endif
