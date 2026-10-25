/* Engine-owned INSDS extension. Never install or expose this definition. */
#ifndef CSOUND_INSDS_PRIVATE_H
#define CSOUND_INSDS_PRIVATE_H

#include "pfield.h"

/* Add independent state as named members here. This struct can grow during
   7.x without moving any public INSDS member or reserving fixed spare slots.
   Each member owns its cleanup; freeing p-fields must not free this struct. */
struct insds_private {
  CS_PFIELDS *pfields;
};

#endif
