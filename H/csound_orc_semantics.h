#ifndef CSOUND_ORC_SEMANTICS_H
#define CSOUND_ORC_SEMANTICS_H

#include "csoundCore.h"
#include "csound_orc.h"

/** Gets short version of opcode name, trimming off anything after '.'. 
 If opname has no '.' in name, simply returns the opname pointer.  
 If the name is truncated, caller is responsible for calling mfree
 on returned value.  Caller should compare the returned value with the
 passed in opname to see if it is different and thus requires mfree'ing. */
char* get_opcode_short_name(CSOUND* csound, char* opname);

PUBLIC OENTRY* find_opcode_new(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound);
PUBLIC OENTRY* find_opcode_exact(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound);
/* find OENTRY with the specified name in opcode list */

OENTRY* find_opcode(CSOUND *, char *);
char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable);

#endif

