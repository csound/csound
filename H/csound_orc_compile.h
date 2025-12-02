/*
    csound_orc_compile.h:

    Copyright (C) 2025 by Victor Lazzarini

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

#ifndef CSOUND_ORC_COMPILE_H
#define CSOUND_ORC_COMPILE_H

int32_t get_pfield(CSOUND *csound, ENGINE_STATE *engineState,
                   INSTRTXT *ip, char *s);
int32_t tree_arg_list_count(TREE *root);
int32_t args_required(char* argString);
char** split_args(CSOUND* csound, char* argString);
char* get_struct_expr_string(CSOUND* csound, TREE* structTree);
CS_VARIABLE *add_global_variable(CSOUND *csound, ENGINE_STATE *engineState,
                                 CS_TYPE *type, char *name, void *typeArg);
void *find_or_add_constant(CSOUND *csound, CS_HASH_TABLE *constantsPool,
                           const char *name, MYFLT value);
void free_instrtxt(CSOUND *csound, INSTRTXT *instrtxt);
int32_t csound_compile_tree(CSOUND *csound, TREE *root, int32_t async);
int32_t csound_compile_orc(CSOUND *csound, const char *str, int32_t async);
uint8_t file_to_int(CSOUND *csound, const char *name);
void query_deprecated_opcode(CSOUND *csound, ORCTOKEN *o);
int32_t query_reversewrite_opcode(CSOUND *csound, ORCTOKEN *o);
void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState);
void merge_state(CSOUND *csound, ENGINE_STATE *engineState,
                 TYPE_TABLE *typetable, OPDS *ids); 
#endif
