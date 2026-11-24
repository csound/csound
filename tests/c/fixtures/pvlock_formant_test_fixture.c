#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include <stdlib.h>
#include "pvlock_formant_test_fixture.h"
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#include "../../../Opcodes/pvlock.c"

void *csound_test_pv_formant_new(CSOUND *csound, int32_t N)
{
  FORMANT_DATA *fd = (FORMANT_DATA *) calloc(1, sizeof(FORMANT_DATA));
  size_t size = (size_t)(N/2 + 2)*sizeof(cs_float);
  int32_t M = N/2;

  if (fd == NULL) return NULL;
  fd->fenv.auxp = csound->Calloc(csound, size);
  fd->ceps.auxp = csound->Calloc(csound, size);
  if (fd->fenv.auxp == NULL || fd->ceps.auxp == NULL) {
    csound_test_pv_formant_free(csound, fd);
    return NULL;
  }
  fd->fwdsetup = csound->RealFFTSetup(csound, M, FFT_FWD);
  fd->invsetup = csound->RealFFTSetup(csound, M, FFT_INV);
  if (fd->fwdsetup == NULL || fd->invsetup == NULL) {
    csound_test_pv_formant_free(csound, fd);
    return NULL;
  }
  return fd;
}

void csound_test_pv_formant_free(CSOUND *csound, void *handle)
{
  FORMANT_DATA *fd = (FORMANT_DATA *) handle;

  if (fd == NULL) return;
  if (fd->fenv.auxp != NULL) csound->Free(csound, fd->fenv.auxp);
  if (fd->ceps.auxp != NULL) csound->Free(csound, fd->ceps.auxp);
  free(fd);
}

void csound_test_pv_formant_lifter(cs_float *ceps, int32_t M, int32_t coefs)
{
  pv_formant_lifter(ceps, M, coefs);
}

void csound_test_pv_formant_analyse(CSOUND *csound, void *handle, int32_t N,
                                    int32_t mode, int32_t coefs,
                                    cs_float *fwin, cs_float *bwin)
{
  pv_formant_analyse(csound, (FORMANT_DATA *) handle, N, mode, coefs,
                     fwin, bwin);
}

void csound_test_pv_formant_restore(void *handle, int32_t N, cs_float *fwin,
                                    cs_float pitch)
{
  pv_formant_restore((FORMANT_DATA *) handle, N, fwin, pitch);
}

void csound_test_pv_formant_get_env(void *handle, cs_float *out, int32_t count)
{
  FORMANT_DATA *fd = (FORMANT_DATA *) handle;
  int32_t k;

  for (k = 0; k < count; k++) out[k] = ((cs_float *) fd->fenv.auxp)[k];
}

void csound_test_pv_formant_set_env(void *handle, const cs_float *in,
                                    int32_t count)
{
  FORMANT_DATA *fd = (FORMANT_DATA *) handle;
  int32_t k;

  for (k = 0; k < count; k++) ((cs_float *) fd->fenv.auxp)[k] = in[k];
}
