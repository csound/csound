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
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301 USA
 */

#ifndef CSOUND_ORC_EXPRESSION_H
#define CSOUND_ORC_EXPRESSION_H 1

#include "csound_orc.h"

CONS_CELL* cs_cons(CSOUND* csound, void* val, CONS_CELL* cons);
CONS_CELL* cs_cons_append(CONS_CELL* cons1, CONS_CELL* cons2);

int is_expression_node(TREE *node);
int is_boolean_expression_node(TREE *node);
int is_statement_expansion_required(TREE* root);

void handle_optional_args(CSOUND *csound, TREE *l);

TREE* expand_if_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable);
TREE* expand_until_statement(CSOUND* csound, TREE* current,
                             TYPE_TABLE* typeTable, int);
TREE* expand_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable);

#endif
