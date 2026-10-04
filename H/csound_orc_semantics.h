/*
    csound_orc_sematics.h:

    Copyright (C) 2013 by Steve Yi

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef CSOUND_ORC_SEMANTICS_H
#define CSOUND_ORC_SEMANTICS_H

#include "csoundCore.h"
#include "csound_orc.h"
#include "csound_orc_structs.h"  /* CS_STRUCT_VAR public layout */

/** Gets short version of opcode name, trimming off anything after '.'.
 If opname has no '.' in name, simply returns the opname pointer.
 If the name is truncated, caller is responsible for calling csoundFree
 on returned value.  Caller should compare the returned value with the
 passed in opname to see if it is different and thus requires csoundFree'ing. */
#include "find_opcode.h"

char *strip_extension(CSOUND *csound, const char *s);
char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable);
char* create_array_arg_type(CSOUND* csound, CS_VARIABLE* arrayVar);
int32_t is_pfield_name(const char *name);
int32_t get_pfield_index(const char *name);

void print_tree(CSOUND *, char *, TREE *);
OENTRIES* find_opcode2(CSOUND*, char*);
char* resolve_opcode_get_outarg(CSOUND* csound,
                                OENTRIES* entries, char* inArgTypes);
void do_baktrace(CSOUND *csound, uint64_t files);
char* get_arg_string_from_tree(CSOUND* csound, TREE* tree,
                               TYPE_TABLE* typeTable);
char* convert_external_to_internal(CSOUND* csound, char* arg);
int32_t check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs);
void handle_optional_args(CSOUND *, TREE *);
char* resolve_opcode_get_outarg(CSOUND* , OENTRIES* , char*);
/* grammar-rule variant of tree_append(); only for use in parser rules */
TREE* parser_append(CSOUND * csound, TREE *first, TREE *newlast);
void add_arg(CSOUND* csound, char* varName, char* annotation,
	     TYPE_TABLE* typeTable, TREE *tree);
void add_array_arg(CSOUND* csound, char* varName, char* annotation,
                          int32_t dimensions,
                          TYPE_TABLE* typeTable);
char *check_annotated_type(CSOUND* csound, OENTRIES* entries,
                           char* outArgTypes);
CS_VARIABLE* find_var_from_pools(CSOUND* csound, const char* varName,
                                 const char* varBaseName, TYPE_TABLE* typeTable);
/* Indexing a variable that is neither an array nor an a-signal is
   delegated to the "##array_get" / "##array_set" / "##array_init" entries
   whose first argument is the variable's own type, so a plugin type can
   provide []. "=" writes through "##array_set", init through "##array_init". */
int32_t type_has_index_opcode(CSOUND* csound, const CS_TYPE* type, const char* opname);
char* resolve_index_get_type(CSOUND* csound, const CS_TYPE* type, TREE* indices, TYPE_TABLE* typeTable);
/* The "##array_get" entry reading type[...]: at perf time, following
   changes, or at init time. NULL, reported, when no single entry fits. */
OENTRY* resolve_index_read_entry(CSOUND* csound, const CS_TYPE* type, TREE* indices,
                                 TYPE_TABLE* typeTable, int32_t perf);
/* The opname entry ("##array_set" or "##array_init") writing type[...];
   *valueType receives the type of the value it takes. */
OENTRY* resolve_index_set_entry(CSOUND* csound, const CS_TYPE* type, const char* opname,
                                TREE* value, TREE* indices, TYPE_TABLE* typeTable,
                                char** valueType);
TREE* verify_tree(CSOUND * csound, TREE *root, TYPE_TABLE* typeTable);
int32_t verify_opcode(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable);
// bison functions
extern int32_t csound_orcget_lineno(void*);
extern char *csound_orcget_current_pointer(void *);



TREE* copy_node_shallow(CSOUND* csound, TREE* tree);
#endif
