/*
 csound_orc_expressions.h:

 Copyright (C) 2013
 Steven Yi

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

#ifndef CSOUND_ORC_EXPRESSION_H
#define CSOUND_ORC_EXPRESSION_H 1

#include "csound_orc.h"

typedef struct {
    TREE* continueTargetIdent;
    TREE* breakTargetIdent;
    TREE* breakTargetLabel;
    int32_t gotoType;
} LOOP_JUMP_TARGETS;

CONS_CELL* cs_cons(CSOUND* csound, void* val, CONS_CELL* cons);
CONS_CELL* cs_cons_append(CONS_CELL* cons1, CONS_CELL* cons2);

int32_t is_expression_node(TREE *node);
int32_t is_boolean_expression_node(TREE *node);
int32_t is_statement_expansion_required(TREE* root);
void handle_optional_args(CSOUND *csound, TREE *l);
TREE* expand_if_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable);
TREE* expand_until_statement(CSOUND* csound, TREE* current,
                             TYPE_TABLE* typeTable, int32_t, LOOP_JUMP_TARGETS* targets);
TREE* expand_switch_statement(CSOUND* csound, TREE* current,TYPE_TABLE* typeTable,
  char* switchArgType);
TREE* expand_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable);
TREE* expand_for_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable, char* arrayArgType,
                           LOOP_JUMP_TARGETS* targets);
char *remove_type_quoting(CSOUND *csound, const char *outype);
TREE * create_opcode_token(CSOUND *csound, char* op);
TREE* convert_break_to_goto(CSOUND* csound, LOOP_JUMP_TARGETS* targets);
TREE* convert_continue_to_goto(CSOUND* csound, LOOP_JUMP_TARGETS* targets);
int expand_struct_array_member_assignment(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable, TREE** anchor);
TREE* expand_struct_array_member_read(CSOUND* csound, TREE* structExpr, int32_t line, uint64_t locn, TYPE_TABLE* typeTable);


#endif
