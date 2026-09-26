/* Native DSSI/LADSPA library lookup. */
#ifndef DSSI4CS_UTILS_H
#define DSSI4CS_UTILS_H
#include "csdl.h"
#define DSSI4CS_DEFAULT_PATH "/usr/local/lib/dssi:/usr/lib/dssi:/usr/lib64/dssi:/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa"
void *dlopenLADSPA(CSOUND *, const char *, int32_t);
#endif
