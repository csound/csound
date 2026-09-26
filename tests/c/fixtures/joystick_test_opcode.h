#ifndef CSOUND_TEST_JOYSTICK_OPCODE_H
#define CSOUND_TEST_JOYSTICK_OPCODE_H
#include "../../../Opcodes/linuxjoystick.h"
#ifdef __cplusplus
extern "C" {
#endif
int32_t csound_test_linuxjoystick_init(CSOUND *, LINUXJOYSTICK *);
int32_t csound_test_linuxjoystick_deinit(CSOUND *, LINUXJOYSTICK *);
int32_t csound_test_linuxjoystick(CSOUND *, LINUXJOYSTICK *);
#ifdef __cplusplus
}
#endif
#endif
