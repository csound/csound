/*
  wasm_opcode_abi.h:

  Frozen wasm32 structure layout used by OPCODE.WASM plugins.

  Top/wasm_plugins.c and wasm/browser/src/dlinit.js read these structures from
  guest memory. These offsets form a frozen ABI. Do not change them without a
  versioned migration for existing plugins. Wasm builds check the current
  structure layout against this file in Top/wasm_opcode_abi_check.c.
*/

#ifndef CSOUND_WASM_OPCODE_ABI_H
#define CSOUND_WASM_OPCODE_ABI_H

#ifndef CSOUNDCORE_H
#error "Include csoundCore.h before Top/wasm_opcode_abi.h"
#endif

#include <stddef.h>

enum {
  WASM32_OENTRY_SIZE = 40,
  WASM32_OPDS_SIZE = 32,
  WASM32_CS_TYPE_SIZE = 32,
  WASM32_VAR_TYPE_OFFSET = 8,
  WASM32_CSOUND_CALLOC_OFFSET = 180,
  WASM32_OPDS_INIT_OFFSET = 12,
  WASM32_OPDS_PERF_OFFSET = 16,
  WASM32_OPDS_DEINIT_OFFSET = 20,
  WASM32_OPDS_INSDS_OFFSET = 28,
  WASM32_INSDS_ESR_OFFSET = 104,
  WASM32_INSDS_KSMPS_OFFSET = 144,
  WASM32_INSDS_EKR_OFFSET = 152,
  WASM32_INSDS_KSMPS_OFFSET_OFFSET = 236,
  WASM32_INSDS_KSMPS_NO_END_OFFSET = 244,
  WASM32_INSDS_PREFIX_SIZE = 256,

  WASM32_OENTRY_NAME_OFFSET = 0,
  WASM32_OENTRY_DSBLOCKSIZE_OFFSET = 4,
  WASM32_OENTRY_FLAGS_OFFSET = 8,
  WASM32_OENTRY_OUTYPES_OFFSET = 12,
  WASM32_OENTRY_INTYPES_OFFSET = 16,
  WASM32_OENTRY_INIT_OFFSET = 20,
  WASM32_OENTRY_PERF_OFFSET = 24,
  WASM32_OENTRY_DEINIT_OFFSET = 28,
  WASM32_OENTRY_USEROPINFO_OFFSET = 32,
  WASM32_OENTRY_DEPRECATED_OFFSET = 36
};

#if defined(__wasm__) && !defined(__wasm64__)
#ifdef __cplusplus
#define CSOUND_WASM_ABI_ASSERT(condition, message) static_assert(condition, message)
#else
#define CSOUND_WASM_ABI_ASSERT(condition, message) _Static_assert(condition, message)
#endif

CSOUND_WASM_ABI_ASSERT(sizeof(void *) == 4, "OPCODE.WASM needs wasm32 pointers");
CSOUND_WASM_ABI_ASSERT(sizeof(OENTRY) == WASM32_OENTRY_SIZE,
                       "OPCODE.WASM OENTRY size changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, opname) == WASM32_OENTRY_NAME_OFFSET,
                       "OPCODE.WASM OENTRY.opname offset changed");
CSOUND_WASM_ABI_ASSERT(
    offsetof(OENTRY, dsblksiz) == WASM32_OENTRY_DSBLOCKSIZE_OFFSET,
    "OPCODE.WASM OENTRY.dsblksiz offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, flags) == WASM32_OENTRY_FLAGS_OFFSET,
                       "OPCODE.WASM OENTRY.flags offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, outypes) == WASM32_OENTRY_OUTYPES_OFFSET,
                       "OPCODE.WASM OENTRY.outypes offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, intypes) == WASM32_OENTRY_INTYPES_OFFSET,
                       "OPCODE.WASM OENTRY.intypes offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, init) == WASM32_OENTRY_INIT_OFFSET,
                       "OPCODE.WASM OENTRY.init offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, perf) == WASM32_OENTRY_PERF_OFFSET,
                       "OPCODE.WASM OENTRY.perf offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OENTRY, deinit) == WASM32_OENTRY_DEINIT_OFFSET,
                       "OPCODE.WASM OENTRY.deinit offset changed");
CSOUND_WASM_ABI_ASSERT(
    offsetof(OENTRY, useropinfo) == WASM32_OENTRY_USEROPINFO_OFFSET,
    "OPCODE.WASM OENTRY.useropinfo offset changed");
CSOUND_WASM_ABI_ASSERT(
    offsetof(OENTRY, deprecated) == WASM32_OENTRY_DEPRECATED_OFFSET,
    "OPCODE.WASM OENTRY.deprecated offset changed");

CSOUND_WASM_ABI_ASSERT(sizeof(OPDS) == WASM32_OPDS_SIZE,
                       "OPCODE.WASM OPDS size changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OPDS, init) == WASM32_OPDS_INIT_OFFSET,
                       "OPCODE.WASM OPDS.init offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OPDS, perf) == WASM32_OPDS_PERF_OFFSET,
                       "OPCODE.WASM OPDS.perf offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OPDS, deinit) == WASM32_OPDS_DEINIT_OFFSET,
                       "OPCODE.WASM OPDS.deinit offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(OPDS, insdshead) == WASM32_OPDS_INSDS_OFFSET,
                       "OPCODE.WASM OPDS.insdshead offset changed");

CSOUND_WASM_ABI_ASSERT(sizeof(CS_TYPE) == WASM32_CS_TYPE_SIZE,
                       "OPCODE.WASM CS_TYPE size changed");
CSOUND_WASM_ABI_ASSERT(CS_VAR_TYPE_OFFSET == WASM32_VAR_TYPE_OFFSET,
                       "OPCODE.WASM variable type offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(CSOUND, Calloc) == WASM32_CSOUND_CALLOC_OFFSET,
                       "OPCODE.WASM CSOUND.Calloc offset changed");

/* The bridge exposes only this prefix. Guest code must not use later fields. */
CSOUND_WASM_ABI_ASSERT(sizeof(INSDS) >= WASM32_INSDS_PREFIX_SIZE,
                       "OPCODE.WASM INSDS prefix no longer fits");
CSOUND_WASM_ABI_ASSERT(offsetof(INSDS, esr) == WASM32_INSDS_ESR_OFFSET,
                       "OPCODE.WASM INSDS.esr offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(INSDS, ksmps) == WASM32_INSDS_KSMPS_OFFSET,
                       "OPCODE.WASM INSDS.ksmps offset changed");
CSOUND_WASM_ABI_ASSERT(offsetof(INSDS, ekr) == WASM32_INSDS_EKR_OFFSET,
                       "OPCODE.WASM INSDS.ekr offset changed");
CSOUND_WASM_ABI_ASSERT(
    offsetof(INSDS, ksmps_offset) == WASM32_INSDS_KSMPS_OFFSET_OFFSET,
    "OPCODE.WASM INSDS.ksmps_offset offset changed");
CSOUND_WASM_ABI_ASSERT(
    offsetof(INSDS, ksmps_no_end) == WASM32_INSDS_KSMPS_NO_END_OFFSET,
    "OPCODE.WASM INSDS.ksmps_no_end offset changed");

#undef CSOUND_WASM_ABI_ASSERT
#endif

#endif
