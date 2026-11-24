#ifndef UDO_H
#define UDO_H

#include "csoundCore.h"
#include "srconvert.h"

typedef struct udo_output_snapshot UDO_OUTPUT_SNAPSHOT;

typedef struct {
    OPCODINFO *opcode_info;
    void    *uopcode_struct;
    INSDS   *parent_ip;
    int32   iflag;
    cs_float  **inargs;
    cs_float   *iobufp_ptrs[12];  /* expandable IV - Oct 26 2002 */ /* was 8 */
} OPCOD_IOBUFS;

typedef struct {                /* IV - Sep 8 2002: new structure: UOPCODE */
    OPDS          h;
    INSDS         *ip, *parent_ip;
    OPCOD_IOBUFS  *buf;
    SR_CONVERTER  **cvt_in;
    SR_CONVERTER  **cvt_out;
    UDO_OUTPUT_SNAPSHOT *output_snapshots;
    cs_float **output_work;
    int32_t isolate_outputs;
    /* special case: the argument list is stored at the end of the */
    /* opcode data structure */
    cs_float         *ar[1];
} UOPCODE;

/* the number of optional outputs defined in entry.c */
#define SUBINSTNUMOUTS  8

typedef struct {                        /* IV - Oct 16 2002 */
    OPDS    h;
    cs_float   *ar[VARGMAX];
    INSDS   *ip, *parent_ip;
    AUXCH   saved_spout;
    OPCOD_IOBUFS    buf;
} SUBINST;

typedef struct {
    OPDS    h;
    cs_float   *i_ksmps;
} SETKSMPS;

typedef struct {
    OPDS    h;
    cs_float   *os;
    cs_float   *in_cvt;
    cs_float   *out_cvt;
} OVSMPLE;

typedef struct {
    OPDS    h;
    cs_float   *args[1];
} XIN;

typedef struct {
    OPDS    h;
    cs_float   *args[OPCODENUMOUTS_MAX];
} XIN_MAX;

typedef struct {
    OPDS    h;
    cs_float   *args[1];
} XOUT;

typedef struct {
    OPDS    h;
    cs_float   *args[OPCODENUMOUTS_MAX];
} XOUT_MAX;

/**
 * Advance past one UDO type specifier in external or internal form
 * (e.g., "k", "k[]", "[k]", ":Struct;", ":Struct;[]", "[:Struct;]").
 */
static inline const char *udo_next_type_token(const char *p) {
  if (p == NULL || *p == '\0') return p;
  while (*p == '[') p++;
  if (*p == ':') {
    p++;
    while (*p != '\0' && *p != ';') p++;
    if (*p == ';') p++;
  } else if (*p != '\0') {
    p++;
  }
  while (*p == ']' || (p[0] == '[' && p[1] == ']')) {
    p += (*p == ']') ? 1 : 2;
  }
  return p;
}

/**
 * Return 1 iff the argument at `index` in `types` is the scalar "K" type.
 */
static inline int32_t udo_arg_is_K(const char *types, int32_t index) {
  const char *p = types;
  if (types == NULL || index < 0) return 0;
  for (int32_t idx = 0; idx < index && *p != '\0'; idx++) {
    const char *next = udo_next_type_token(p);
    if (next == p) return 0;
    p = next;
  }
  if (*p == '\0') return 0;
  return (udo_next_type_token(p) - p == 1 && *p == 'K');
}

/**
 * Compare two UDO type signature strings token-by-token.
 * Allows scalar 'k' to match scalar 'K' (k-rate input accepting K-type pass-by-ref).
 * Returns 0 if signatures match, 1 if they don't.
 */
static inline int32_t inargs_match(const char *intypes, const char *inargs) {
  const char *p = intypes;
  const char *q = inargs;
  if (intypes == NULL || inargs == NULL) return 1;
  if (strcmp(intypes, inargs) == 0) return 0;
  while (*p != '\0' && *q != '\0') {
    const char *np = udo_next_type_token(p);
    const char *nq = udo_next_type_token(q);
    size_t lp = (size_t)(np - p);
    size_t lq = (size_t)(nq - q);
    if (lp == 0 || lq == 0) return 1;
    if (lp != lq) return 1;
    if (memcmp(p, q, lp) != 0) {
      if (!(lp == 1 && *p == 'k' && *q == 'K')) return 1;
    }
    p = np;
    q = nq;
  }
  return (*p != '\0' || *q != '\0') ? 1 : 0;
}

int32_t useropcd(CSOUND *, UOPCODE *p);
int32_t useropcdset(CSOUND *, UOPCODE *p);
int32_t useropcd_local_ksmps(CSOUND *, UOPCODE*);
int32_t useropcd_pass_by_copy(CSOUND *, UOPCODE*);
int32_t useropcd_pass_by_ref(CSOUND *, UOPCODE *);
void free_user_opcode_buffers(CSOUND *, UOPCODE *);
void recycle_init_only_udo_instances(CSOUND *, INSDS *);
void build_user_opcode_rewire_plan(CSOUND *, OPCODINFO *);
void free_opcode_info_chain(CSOUND *);
void csound_publish_latest_udos(CSOUND *);

/* Storage a pass-by-ref UDO argument was rewired onto (a caller argument or
   output work buffer), or NULL for an ordinary local or a pass-by-copy call.
   The returned pointer is borrowed and only valid while the instance is
   active. */
cs_float *user_opcode_ref_arg_storage(const UOPCODE *p, const char *varName);

/* Call before an opcode object's arguments change after init, while p->ar
   still holds the old ones. A pass-by-ref body works on xin's copy of an
   i-rate constant it writes, and a rebind keeps that copy as init left it.
   A constant given where the call before gave other storage starts the copy
   from its value, as a body rebound to a variable reads what that holds. */
void user_opcode_rebind_constants(UOPCODE *p, cs_float **inargs,
                                  int32_t count);

/* Rewire a pass-by-ref UDO body, and the UDOs nested in it, onto the current
   p->ar locations after an opcode object's arguments change following init. */
int32_t user_opcode_rebind_args(CSOUND *csound, UOPCODE *p);

#endif
