/*
  csound_orc_verify.c:

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

#include "csoundCore.h"
#include "csound_orc.h"
#include "csound_type_system.h"

int is_wildcard_type(char* typeIdent) {
  return strchr("?.*", *typeIdent) != NULL;
}

int is_vararg_input_type(char* typeIdent) {
  return strlen(typeIdent) == 1 &&
    strchr("MNmWyzZ", *typeIdent) != NULL;
}

int is_optarg_input_type(char* typeIdent) {
  return strchr("?IOoPpVvhJj", *typeIdent) != NULL;
}

int is_init_opcode(char* opname) {
    return strncmp(opname, "init", 4) == 0;
}

int is_internal_array_opcode(char* opname) {
    return strncmp(opname, "##array_", 8) == 0;
}

int is_unary_token_type(int tokenType) {
  return (tokenType == S_UNOT) ||
    (tokenType == S_UMINUS) ||
    (tokenType == S_UPLUS);
}

int is_legacy_t_rate_ident(char* ident) {
  char* token = ident;

  if (*token == '#') {
    token += 1;
  }
  if (*token == 'g') {
    token += 1;
  }
  return *token == 't';
}


int check_satisfies_arate_input(
    char* typeIdent
) {
    return strchr("aXMNyx", *typeIdent) != NULL;
}

int check_satisfies_krate_input(
    char* typeIdent
) {
    return strchr("ckmxzpXUOJVPN", *typeIdent) != NULL;
}

int check_satisfies_expected_input(
    CS_TYPE* cstype,
    char* typeIdent,
    int isArray
) {
    if (is_wildcard_type(typeIdent)) {
        return 1;
    }

    if (
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_C) &&
        strchr("Sf", *typeIdent) == NULL // fewer letters to rule out than include
    ) {
        return 1;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_A)) {
        return check_satisfies_arate_input(typeIdent);
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_K)) {
        return check_satisfies_krate_input(typeIdent);
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_B)) {
        return strchr("B", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_b)) {
        return strchr("Bb", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_L)) {
        return strchr("l", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_S)) {
        return strchr("SUNTW", *typeIdent) != NULL;
    } else if (
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_I) ||
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_R) ||
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_P)
    ) {
        return isArray ?
            strchr("icoXUNcmI", *typeIdent) != NULL :
            strchr("ickoxXUOJjVPNTmIz", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_F)) {
        return strchr("f", *typeIdent) != NULL;
    }

    return 0;
}

int check_satisfies_alternating_z_input(
    char* typeIdent,
    int argIndex
) {
    if (argIndex % 2 == 0) {
        return check_satisfies_krate_input(typeIdent);
    } else {
        return check_satisfies_arate_input(typeIdent);
    }
}

int check_satisfies_expected_output(
    CS_TYPE* cstype,
    char* typeIdent
) {
    if (is_wildcard_type(typeIdent)) {
        return 1;
    }

    if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_C)) {
        return 1;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_A)) {
        return strchr("amXN", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_K)) {
        return strchr("kzXN", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_B)) {
        return strchr("B", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_b)) {
        return strchr("Bb", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_L)) {
        return strchr("l", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_S)) {
        return strchr("SNI", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_R)) {
        return strchr("zXNicmIr", *typeIdent) != NULL;
    } else if (
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_I) ||
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_R) ||
        cstype == ((CS_TYPE*) &CS_VAR_TYPE_P)
    ) {
        return strchr("zXNicmI", *typeIdent) != NULL;
    } else if (cstype == ((CS_TYPE*) &CS_VAR_TYPE_F)) {
        return strchr("f", *typeIdent) != NULL;
    }

    return 0;

}
