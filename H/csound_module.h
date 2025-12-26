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

/* Import types */
typedef enum {
    IMPORT_MODULE,          /* import lib.module */
    IMPORT_MODULE_AS,       /* import lib.module as alias */
    IMPORT_FROM,            /* from lib.module import item1, item2 */
    IMPORT_FROM_ALL         /* from lib.module import * */
} IMPORT_TYPE;

/* Forward declarations */
typedef struct cs_import_item CS_IMPORT_ITEM;
typedef struct cs_import CS_IMPORT;

/* Import item structure */
struct cs_import_item {
    char *original_name;           /* Name in imported module */
    char *local_name;              /* Name in current module */
    bool is_wildcard;              /* True for import * */
};

/* Import statement structure */
struct cs_import {
    IMPORT_TYPE type;
    char *module_path;             /* Path being imported */
    char *module_alias;            /* For "import module as alias" */
    CS_IMPORT_ITEM *items;         /* For "from module import a, b" */
    int32_t item_count;
    bool is_wildcard;              /* for "from module import *" */
};


/* Module structure */
struct cs_module {
    char *name;                    /* Module name (e.g., "oscillators") */
    char *file_path;               /* Full path to source file */
    char *normalized_path;         /* Normalized import path (e.g., "lib.oscillators") */
    CS_HASH_TABLE *opcodes;        /* UDOs in this module */
    CS_HASH_TABLE *module_vars;    /* All module-level variables */
    CS_VAR_POOL *varPool;          /* Module-level variable pool for g-variables and @global */
    ENGINE_STATE *engineState;     /* Module's engine state (constants, strings, etc.) */
    TREE *ast;                     /* Cached AST for singleton behavior */
    INSTRTXT *instr0;              /* Module's instrument 0 for global initialization */
    bool is_compiled;              /* Compilation complete flag */
    bool is_virtual;               /* C opcode module flag */
    int32_t ref_count;             /* Reference counting for imports */
    CS_MODULE **imports;           /* Imported modules */
    CS_IMPORT **import_info;       /* Import metadata (selective imports, aliases) */
    int32_t import_count;
    int32_t import_capacity;       /* Capacity of imports array */
};

/* Public API functions */

/**
 * Get the module system state for a Csound instance
 */
PUBLIC MODULE_STATE* csoundGetModuleState(CSOUND *csound);

/**
 * Create and initialize the root module for the main CSD
 */
PUBLIC CS_MODULE* csoundCreateRootModule(CSOUND *csound);

/**
 * Initialize the module system for a Csound instance
 */
PUBLIC void csoundInitializeModuleSystem(CSOUND *csound);

/**
 * Cleanup the module system for a Csound instance
 */
PUBLIC void csoundCleanupModuleSystem(CSOUND *csound);

/**
 * Get or load a module by path (implements singleton behavior)
 * Returns existing compiled module or loads and compiles new one
 */
PUBLIC CS_MODULE* csoundGetOrLoadModule(CSOUND *csound, const char *path);

/**
 * Add a search path for module resolution
 */
PUBLIC int csoundAddModuleSearchPath(CSOUND *csound, const char *path);

/**
 * Resolve a module import path to a file path
 * Returns allocated string that must be freed by caller
 */
PUBLIC char* csoundResolveModulePath(CSOUND *csound, const char *import_path);

/**
 * Find a variable in a module
 */
PUBLIC CS_VARIABLE* csoundFindModuleVariable(CSOUND *csound,
                                            CS_MODULE *module,
                                            const char *name);

/**
 * Find an opcode in a module
 */
PUBLIC OENTRY* csoundFindModuleOpcode(CSOUND *csound,
                                      CS_MODULE *module,
                                      const char *name);

/**
 * Check if an item exists in a module (variable or opcode)
 */
PUBLIC bool csoundModuleHasItem(CSOUND *csound,
                                CS_MODULE *module,
                                const char *name);

/**
 * Create a new virtual module (for C opcode organization)
 */
PUBLIC CS_MODULE* csoundCreateVirtualModule(CSOUND *csound, const char *name);

/**
 * Register an opcode in a virtual module
 */
PUBLIC int csoundRegisterVirtualOpcode(CSOUND *csound,
                                       CS_MODULE *module,
                                       const char *opname,
                                       int (*init)(CSOUND *, void *),
                                       int (*perf)(CSOUND *, void *),
                                       int (*deinit)(CSOUND *, void *));

/* Internal helper functions (not part of public API) */

/**
 * Load and compile a module from file
 */
PUBLIC CS_MODULE* csoundLoadModuleFromFile(CSOUND *csound, const char *path);

/**
 * Process import statements in a module AST
 */
PUBLIC void csoundProcessImportStatements(CSOUND *csound,
                                         CS_MODULE *module,
                                         TREE *ast);

/**
 * Register module variables and opcodes from compiled AST
 */
PUBLIC void csoundRegisterModuleItems(CSOUND *csound, CS_MODULE *module);

/**
 * Find a module by its import alias
 * Returns the module if found, NULL otherwise
 */
PUBLIC CS_MODULE* csoundFindModuleByAlias(CSOUND *csound, const char *alias);

/**
 * Find an imported module by name or alias
 */
PUBLIC CS_MODULE* csoundFindImportedModule(CSOUND *csound,
                                          CS_MODULE *current,
                                          const char *name);

/**
 * Add an import to a module's import list
 */
PUBLIC int csoundAddModuleImport(CSOUND *csound,
                                CS_MODULE *module,
                                CS_MODULE *imported,
                                const char *alias);

/**
 * Register a specific item from a module (for "from module import item")
 */
PUBLIC void csoundRegisterModuleItem(CSOUND *csound,
                                    CS_MODULE *target_module,
                                    CS_MODULE *source_module,
                                    const char *item_name,
                                    const char *alias);

/**
 * Check if a named item is allowed to be imported from a module
 * Returns 1 if allowed (wildcard or in explicit list), 0 if not
 */
PUBLIC int csoundIsItemImportAllowed(CSOUND *csound,
                                     CS_MODULE *importing_module,
                                     CS_MODULE *source_module,
                                     const char *item_name);

/**
 * Create a CS_IMPORT structure for tracking import metadata
 */
PUBLIC CS_IMPORT* csoundCreateImport(CSOUND *csound,
                                     IMPORT_TYPE type,
                                     const char *module_path,
                                     const char *alias,
                                     int is_wildcard);

/**
 * Add an item to a CS_IMPORT's item list
 */
PUBLIC void csoundAddImportItem(CSOUND *csound,
                                CS_IMPORT *import_info,
                                const char *original_name,
                                const char *local_name);

#ifdef __cplusplus
}
#endif

#endif /* CSOUND_MODULE_H */
