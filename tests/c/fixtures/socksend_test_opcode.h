#ifndef CSOUND_TEST_SOCKSEND_OPCODE_H
#define CSOUND_TEST_SOCKSEND_OPCODE_H
#include "../../../Opcodes/socksend.h"
#ifdef __cplusplus
extern "C" {
#endif
extern void (*csound_test_udp_capture)(const void *, size_t);
int32_t csound_test_init_send(CSOUND *, SOCKSEND *);
int32_t csound_test_init_sendS(CSOUND *, SOCKSENDS *);
int32_t csound_test_init_send_Str(CSOUND *, SOCKSENDT *);
int32_t csound_test_send_send(CSOUND *, SOCKSEND *);
int32_t csound_test_send_send_k(CSOUND *, SOCKSEND *);
int32_t csound_test_send_sendS(CSOUND *, SOCKSENDS *);
int32_t csound_test_send_send_Str(CSOUND *, SOCKSENDT *);
int32_t csound_test_socksend_deinit(CSOUND *, SOCKSEND *);
int32_t csound_test_socksends_deinit(CSOUND *, SOCKSENDS *);
int32_t csound_test_osc_send2_init(CSOUND *, OSCSEND2 *);
int32_t csound_test_osc_send2(CSOUND *, OSCSEND2 *);
int32_t csound_test_oscsend_deinit(CSOUND *, OSCSEND2 *);
int32_t csound_test_oscbundle_init(CSOUND *, OSCBUNDLE *);
int32_t csound_test_oscbundle_perf(CSOUND *, OSCBUNDLE *);
int32_t csound_test_oscbundle_deinit(CSOUND *, OSCBUNDLE *);
#ifdef __cplusplus
}
#endif
#endif
