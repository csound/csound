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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 */

int is_expression_node(TREE *node);
int is_boolean_expression_node(TREE *node);
int is_statement_expansion_required(TREE* root);

TREE* expand_if_statement(CSOUND* csound, TREE* current);
TREE* expand_until_statement(CSOUND* csound, TREE* current);
TREE* expand_statement(CSOUND* csound, TREE* current);