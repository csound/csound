#ifndef CSOUND_TEST_PVLOCK_FORMANT_H
#define CSOUND_TEST_PVLOCK_FORMANT_H
#include "csoundCore.h"
#ifdef __cplusplus
extern "C" {
#endif
void *csound_test_pv_formant_new(CSOUND *csound, int32_t N);
void csound_test_pv_formant_free(CSOUND *csound, void *handle);
void csound_test_pv_formant_lifter(cs_float *ceps, int32_t M, int32_t coefs);
void csound_test_pv_formant_analyse(CSOUND *csound, void *handle, int32_t N,
                                    int32_t mode, int32_t coefs,
                                    cs_float *fwin, cs_float *bwin);
void csound_test_pv_formant_restore(void *handle, int32_t N, cs_float *fwin,
                                    cs_float pitch);
void csound_test_pv_formant_get_env(void *handle, cs_float *out, int32_t count);
void csound_test_pv_formant_set_env(void *handle, const cs_float *in,
                                    int32_t count);
int32_t csound_test_pv_formant_active(void *handle, int32_t N,
                                      cs_float keepform, cs_float kcoefs,
                                      cs_float kpitch, int32_t *mode,
                                      int32_t *coefs);
cs_float csound_test_pv_formant_max_gain(void);
#ifdef __cplusplus
}
#endif
#endif
