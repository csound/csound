#ifndef CSOUND_ORC_SEMANTICS_H
#define CSOUND_ORC_SEMANTICS_H

#include "csoundCore.h"
#include "csound_orc.h"

/** Gets short version of opcode name, trimming off anything after '.'.
 If opname has no '.' in name, simply returns the opname pointer.
 If the name is truncated, caller is responsible for calling mfree
 on returned value.  Caller should compare the returned value with the
 passed in opname to see if it is different and thus requires mfree'ing. */
#include "find_opcode.h"
char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable);

#endif
