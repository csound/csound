#ifndef UDO_H
#define UDO_H

#include "csoundCore.h"
#include "srconvert.h"

typedef struct {
    OPCODINFO *opcode_info;
    void    *uopcode_struct;
    INSDS   *parent_ip;
    int32   iflag;
    MYFLT  **inargs;
    MYFLT   *iobufp_ptrs[12];  /* expandable IV - Oct 26 2002 */ /* was 8 */
} OPCOD_IOBUFS;

typedef struct {                /* IV - Sep 8 2002: new structure: UOPCODE */
    OPDS          h;
    INSDS         *ip, *parent_ip;
    OPCOD_IOBUFS  *buf;
    SR_CONVERTER  *cvt_in[OPCODENUMOUTS_MAX];
    SR_CONVERTER  *cvt_out[OPCODENUMOUTS_MAX];
    /* special case: the argument list is stored at the end of the */
    /* opcode data structure */
    MYFLT         *ar[1];
} UOPCODE;

/* the number of optional outputs defined in entry.c */
#define SUBINSTNUMOUTS  8

typedef struct {                        /* IV - Oct 16 2002 */
    OPDS    h;
    MYFLT   *ar[VARGMAX];
    INSDS   *ip, *parent_ip;
    AUXCH   saved_spout;
    OPCOD_IOBUFS    buf;
} SUBINST;

typedef struct {
    OPDS    h;
    MYFLT   *i_ksmps;
} SETKSMPS;

typedef struct {
    OPDS    h;
    MYFLT   *os;
    MYFLT   *in_cvt;
    MYFLT   *out_cvt;
} OVSMPLE;

typedef struct {
    OPDS    h;
    MYFLT   *args[1];
} XIN;

typedef struct {
    OPDS    h;
    MYFLT   *args[OPCODENUMOUTS_LOW];
} XIN_LOW;

typedef struct {
    OPDS    h;
    MYFLT   *args[OPCODENUMOUTS_HIGH];
} XIN_HIGH;

typedef struct {
    OPDS    h;
    MYFLT   *args[OPCODENUMOUTS_MAX];
} XIN_MAX;

typedef struct {
    OPDS    h;
    MYFLT   *args[1];
} XOUT;

typedef struct {
    OPDS    h;
    MYFLT   *args[OPCODENUMOUTS_LOW];
} XOUT_LOW;

typedef struct {
    OPDS    h;
    MYFLT   *args[OPCODENUMOUTS_HIGH];
} XOUT_HIGH;

typedef struct {
    OPDS    h;
    MYFLT   *args[OPCODENUMOUTS_MAX];
} XOUT_MAX;

/**
 * Compare two UDO type signature strings for compatibility.
 * Allows 'k' to match 'K' (k-rate input accepting K-type pass-by-reference).
 * Returns 0 if signatures match, 1 if they don't.
 */
static inline int32_t inargs_match(const char *intypes, const char *inargs) {
  int32_t i;
  if (intypes == NULL || inargs == NULL) return 1;
  if (strcmp(intypes, inargs) != 0) {
    for (i = 0; intypes[i] != 0 && inargs[i] != 0; i++) {
      if (intypes[i] != inargs[i]) {
        if (intypes[i] == 'k' && inargs[i] == 'K') continue;
        else return 1;
      }
    }
    if (intypes[i] != 0 || inargs[i] != 0) return 1;
  }
  return 0;
}

int32_t useropcd(CSOUND *, UOPCODE *p);
int32_t useropcdset(CSOUND *, UOPCODE *p);
int32_t useropcd_local_ksmps(CSOUND *, UOPCODE*);
int32_t useropcd_pass_by_copy(CSOUND *, UOPCODE*);
int32_t useropcd_pass_by_ref(CSOUND *, UOPCODE *);
void csoundBuildUserOpcodeRewirePlan(CSOUND *, OPCODINFO *);
void csoundFreeOpcodeInfoChain(CSOUND *);

#endif
