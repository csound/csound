#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
extern int csound_test_stsend_connect(SOCKET, const struct sockaddr *, int);
extern int csound_test_stsend_send(SOCKET, const char *, int, int);
#else
#include <sys/socket.h>
extern int csound_test_stsend_connect(int, const struct sockaddr *, socklen_t);
extern ssize_t csound_test_stsend_send(int, const void *, size_t, int);
#endif
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define connect csound_test_stsend_connect
#define send csound_test_stsend_send
#include "../../../Opcodes/socksend.c"

OENTRY *csound_test_stsend_opcode(void)
{
    for (size_t i = 0; i < sizeof(socksend_localops) / sizeof(OENTRY); i++)
      if (strcmp(socksend_localops[i].opname, "stsend") == 0)
        return &socksend_localops[i];
    return NULL;
}
#endif
