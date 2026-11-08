/*
    csound_module.h:

    Copyright (C) 2025 Csound Developers

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

#ifndef CSOUND_MODULE_H
#define CSOUND_MODULE_H

#include "csoundCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cs_import_item {
    char *original_name;
    char *local_name;
} CS_IMPORT_ITEM;

typedef struct cs_import {
    bool is_wildcard;
    CS_IMPORT_ITEM *items;
    int32_t item_count;
} CS_IMPORT;

struct cs_module {
    CS_MODULE *owner;              /* Root edits share an identity, not parser storage. */
    CS_MODULE *compiler_scope;
    char *name;
    char *file_path;                /* Canonical file identity; NULL for root. */
    int32_t id;
    CS_HASH_TABLE *opcodes;        /* Name -> complete OENTRY list. */
    CS_HASH_TABLE *aliases;        /* Import aliases in this module only. */
    CS_HASH_TABLE *types;
    CS_VAR_POOL *varPool;
    TREE *ast;
    INSTRTXT *instr0;
    bool is_loading;
    bool is_compiled;
    bool is_initialized;
    CS_MODULE **imports;
    CS_IMPORT **import_info;
    int32_t import_count;
};

typedef struct cs_module_update {
    CS_MODULE *module, *scope, *previous_scope;
    CS_HASH_TABLE *opcodes, *aliases, *types;
    CS_MODULE **imports;
    CS_IMPORT **import_info;
    int32_t import_count;
    bool accepted;
} CS_MODULE_UPDATE;
CS_MODULE_UPDATE *csoundBeginModuleUpdate(CSOUND *csound);
void csoundAcceptModuleUpdate(CS_MODULE_UPDATE *update);
void csoundFinishModuleUpdate(CSOUND *csound, CS_MODULE_UPDATE *update, int commit);
static inline CS_MODULE *csoundModuleOwner(CS_MODULE *module) {
    return module != NULL && module->owner != NULL ? module->owner : module;
}

MODULE_STATE *csoundGetModuleState(CSOUND *csound);
CS_MODULE *csoundCreateRootModule(CSOUND *csound);
CS_MODULE *csoundGetOrLoadModule(CSOUND *csound, const char *path);
CS_VARIABLE *csoundFindModuleVariable(CSOUND *csound, CS_MODULE *module,
                                     const char *name);
CS_VARIABLE *csoundFindSystemVariable(CSOUND *csound, const char *name);
CONS_CELL *csoundFindModuleOpcodes(CSOUND *csound, CS_MODULE *module,
                                  const char *name);
CS_MODULE *csoundFindModuleByAlias(CSOUND *csound, const char *alias);
int csoundProcessImportStatements(CSOUND *csound, CS_MODULE *source,
                                  TREE *tree, CS_VAR_POOL *pool);
int csoundIsItemImportAllowed(CSOUND *csound, CS_MODULE *target,
                              CS_MODULE *source, const char *name);
char *csoundModuleInstrumentName(CSOUND *csound, CS_MODULE *module,
                                 const char *name);

#ifdef __cplusplus
}
#endif
#endif
