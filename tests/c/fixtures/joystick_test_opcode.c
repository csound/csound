#if defined(__linux__) || defined(CSOUND_TEST_JOYSTICK)
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "joystick_test_opcode.h"
#include <fcntl.h>
#include <sys/ioctl.h>
extern int csound_test_joystick_open(const char *, int, ...);
extern int csound_test_joystick_close(int);
extern int csound_test_joystick_ioctl(int, unsigned long, ...);
extern ssize_t csound_test_joystick_read(int, void *, size_t);
#define open csound_test_joystick_open
#define close csound_test_joystick_close
#define ioctl csound_test_joystick_ioctl
#define read csound_test_joystick_read
#undef LINKAGE
#define LINKAGE
#include "../../../Opcodes/linuxjoystick.c"

int32_t csound_test_linuxjoystick_init(CSOUND *csound, LINUXJOYSTICK *p)
{
    return linuxjoystick_init(csound, p);
}

int32_t csound_test_linuxjoystick_deinit(CSOUND *csound, LINUXJOYSTICK *p)
{
    return linuxjoystick_deinit(csound, p);
}

int32_t csound_test_linuxjoystick(CSOUND *csound, LINUXJOYSTICK *p)
{
    return linuxjoystick(csound, p);
}

OENTRY *csound_test_joystick_opcode(void)
{
    return &localops[0];
}
#endif
