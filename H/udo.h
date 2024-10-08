#ifndef UDO_H
#define UDO_H

#include "csoundCore.h"
#include "srconvert.h"

typedef struct {
    OPCODINFO *opcode_info;
    void    *uopcode_struct;
    INSDS   *parent_ip;
    int32   iflag;
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

/* IV - Sep 8 2002: added opcodes: xin, xout, and setksmps */

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

int32_t useropcd1(CSOUND *, UOPCODE*);
int32_t useropcd2(CSOUND *, UOPCODE*);
int32_t useropcd_passByRef(CSOUND *, UOPCODE*);

#endif
