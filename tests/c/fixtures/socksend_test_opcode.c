#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "socksend_test_opcode.h"
void (*csound_test_udp_capture)(const void *, size_t);
#if defined(WIN32) && !defined(__CYGWIN__)
static int capture_packet(SOCKET socket, const char *data, int size, int flags,
                          const struct sockaddr *address, int address_size)
#else
static ssize_t capture_packet(int socket, const void *data, size_t size, int flags,
                              const struct sockaddr *address, socklen_t address_size)
#endif
{
    (void)socket; (void)flags; (void)address; (void)address_size;
    csound_test_udp_capture(data, (size_t)size);
    return size;
}
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#define sendto capture_packet
#include "../../../Opcodes/socksend.c"
#undef sendto

int32_t csound_test_init_send(CSOUND *csound, SOCKSEND *p)
{
    return init_send(csound, p);
}

int32_t csound_test_init_sendS(CSOUND *csound, SOCKSENDS *p)
{
    return init_sendS(csound, p);
}

int32_t csound_test_init_send_Str(CSOUND *csound, SOCKSENDT *p)
{
    return init_send_Str(csound, p);
}

int32_t csound_test_send_send(CSOUND *csound, SOCKSEND *p)
{
    return send_send(csound, p);
}

int32_t csound_test_send_send_k(CSOUND *csound, SOCKSEND *p)
{
    return send_send_k(csound, p);
}

int32_t csound_test_send_sendS(CSOUND *csound, SOCKSENDS *p)
{
    return send_sendS(csound, p);
}

int32_t csound_test_send_send_Str(CSOUND *csound, SOCKSENDT *p)
{
    return send_send_Str(csound, p);
}

int32_t csound_test_socksend_deinit(CSOUND *csound, SOCKSEND *p)
{
    return socksend_deinit(csound, p);
}

int32_t csound_test_socksends_deinit(CSOUND *csound, SOCKSENDS *p)
{
    return socksends_deinit(csound, p);
}

int32_t csound_test_osc_send2_init(CSOUND *csound, OSCSEND2 *p)
{
    return osc_send2_init(csound, p);
}

int32_t csound_test_osc_send2(CSOUND *csound, OSCSEND2 *p)
{
    return osc_send2(csound, p);
}

int32_t csound_test_oscsend_deinit(CSOUND *csound, OSCSEND2 *p)
{
    return oscsend_deinit(csound, p);
}

int32_t csound_test_oscbundle_init(CSOUND *csound, OSCBUNDLE *p)
{
    return oscbundle_init(csound, p);
}

int32_t csound_test_oscbundle_perf(CSOUND *csound, OSCBUNDLE *p)
{
    return oscbundle_perf(csound, p);
}

int32_t csound_test_oscbundle_deinit(CSOUND *csound, OSCBUNDLE *p)
{
    return oscbundle_deinit(csound, p);
}
#endif
