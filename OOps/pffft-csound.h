/* Private PFFFT build settings. Keep the installed H/pffft.h API intact. */
#ifndef CSOUND_PFFFT_H
#define CSOUND_PFFFT_H

#include "float-version.h"
#define PFFFT_STATIC_DEFINE

/* Use only instructions enabled by the target, including plain WASI builds.
   PFFFT_SIMD_DISABLE still takes precedence for scalar-only targets. */
#if defined(__ARM_NEON) || defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define PFFFT_ENABLE_NEON
#endif
#if defined(__wasm_simd128__)
#define PFFFT_ENABLE_WASM
#endif

#include "../third_party/pffft/include/pffft/pffft.h"
#include "../third_party/pffft/include/pffft/pffft_double.h"

#ifdef USE_DOUBLE
typedef PFFFTD_Setup CS_PFFFT_Setup;
#define cs_pffft_new_setup pffftd_new_setup
#define cs_pffft_destroy_setup pffftd_destroy_setup
#define cs_pffft_transform_ordered pffftd_transform_ordered
#else
typedef PFFFT_Setup CS_PFFFT_Setup;
#define cs_pffft_new_setup pffft_new_setup
#define cs_pffft_destroy_setup pffft_destroy_setup
#define cs_pffft_transform_ordered pffft_transform_ordered
#endif
#endif
