/*
  csound_orc_verify.h:

  Copyright (C) 2023

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

#include "csound_data_structures.h"

#ifndef CSOUND_ORC_VERIFY_H
#define CSOUND_ORC_VERIFY_H 1

int is_wildcard_type(char*);
int is_vararg_output_type(char*);
int is_vararg_input_type(char*);
int is_optarg_input_type(char*);
int is_init_opcode(char*);
int is_internal_array_opcode(char*);
int is_unary_token_type(int);
int is_legacy_t_rate_ident(char*);
int check_satisfies_arate_input(char*);
int check_satisfies_krate_input(char*);
int check_satisfies_expected_input(CS_TYPE*, char*);
int check_satisfies_alternating_z_input(char*, int);
int check_satisfies_expected_output(CS_TYPE*, char*);

#endif
