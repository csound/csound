/*
    csound_module.c:

    Copyright (C) 2024 Csound Developers

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
#include "csound_module.h"
#include "csound_orc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/* Forward declarations for compilation functions */
TREE *csoundParseOrc(CSOUND *csound, const char *str);
int32_t csoundCompileTree(CSOUND *csound, TREE *root, int32_t async);
void csoundDeleteTree(CSOUND *csound, TREE *tree);


PUBLIC MODULE_STATE* csoundGetModuleState(CSOUND *csound)
{
    if (csound->module_state == NULL) {
        csound->module_state = (MODULE_STATE*)csound->Calloc(csound, sizeof(MODULE_STATE));
    }
    return csound->module_state;
}

/* Create and initialize the root module for the main CSD
 *
 * The root module represents the main CSD file and is treated like any other
 * module, with its own variable pool for module-level g-variables and @global
 * variables. This provides consistent scoping semantics between the main CSD
 * and imported modules.
 */
PUBLIC CS_MODULE* csoundCreateRootModule(CSOUND *csound)
{
    MODULE_STATE *state = csoundGetModuleState(csound);

    if (state->root_module != NULL) {
        return state->root_module;  /* Already created */
    }

    CS_MODULE *root = (CS_MODULE*)csound->Calloc(csound, sizeof(CS_MODULE));
    root->name = csoundStrdup(csound, "<root>");
    root->file_path = NULL;
    root->normalized_path = csoundStrdup(csound, "<root>");

    /* Root module is treated like any other module */
    root->opcodes = cs_hash_table_create(csound);
    root->module_vars = cs_hash_table_create(csound);
    root->ast = NULL;
    root->is_compiled = 0;
    root->is_virtual = 0;
    root->ref_count = 1;
    root->imports = NULL;
    root->import_count = 0;
    root->import_capacity = 0;

    /* Create module-level variable pool
     * This pool will hold the main CSD's g-variables and @global variables
     * Parent will be set to engineState->varPool after it's created */
    root->varPool = csoundCreateVarPool(csound);
    /* Note: parent chain will be set up later in csound_compile_tree
     * after engineState->varPool is initialized */
    root->varPool->parent = NULL;

    state->root_module = root;
    state->current_module = root;

    return root;
}

/* Note: Text-parsing UDO discovery functions have been removed.
 * UDOs are now compiled automatically when the module's AST is processed
 * by csound_compile_tree(), following the standard Csound compilation path.
 * This matches the pre-module behavior where UDOs are compiled as part of
 * the normal AST traversal. */

static void csoundInitModuleSystem(CSOUND *csound)
{
    MODULE_STATE *state = csoundGetModuleState(csound);

    /* Initialize module system fields if not already done */
    if (state->modules == NULL) {
        state->modules = cs_hash_table_create(csound);
        state->module_search_paths = cs_hash_table_create(csound);
        state->import_aliases = cs_hash_table_create(csound);
        state->module_stack_size = 0;
        state->module_stack_capacity = 16;
        state->module_stack = (CS_MODULE**)csound->Calloc(csound,
                                     sizeof(CS_MODULE*) * state->module_stack_capacity);

        /* Create global module for backward compatibility */
        state->global_module = csound->Calloc(csound, sizeof(CS_MODULE));
        state->global_module->name = csound->Strdup(csound, "global");
        state->global_module->normalized_path = csound->Strdup(csound, "global");
        state->global_module->is_compiled = true;
        state->global_module->is_virtual = true;
        state->global_module->ref_count = 1;
    }
}

PUBLIC void csoundInitializeModuleSystem(CSOUND *csound)
{
    csoundInitModuleSystem(csound);
}

PUBLIC void csoundCleanupModuleSystem(CSOUND *csound)
{
    MODULE_STATE *state = csound->module_state;
    if (state == NULL) return;

    if (state->modules != NULL) {
        /* TODO: Clean up all modules */
        cs_hash_table_free(csound, state->modules);
        state->modules = NULL;
    }

    if (state->module_search_paths != NULL) {
        cs_hash_table_free(csound, state->module_search_paths);
        state->module_search_paths = NULL;
    }

    if (state->import_aliases != NULL) {
        cs_hash_table_free(csound, state->import_aliases);
        state->import_aliases = NULL;
    }

    if (state->module_stack != NULL) {
        csound->Free(csound, state->module_stack);
        state->module_stack = NULL;
    }

    state->current_module = NULL;
    state->global_module = NULL;

    /* Free the state structure itself */
    csound->Free(csound, state);
    csound->module_state = NULL;
}

/* Helper function to strip quotes from string paths */
static char* strip_quotes(CSOUND *csound, const char *str)
{
    if (str == NULL) return NULL;

    size_t len = strlen(str);

    /* Check if string is quoted */
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        char *result = csound->Malloc(csound, len - 1);
        strncpy(result, str + 1, len - 2);
        result[len - 2] = '\0';
        return result;
    }

    /* Not quoted, return a copy */
    return csound->Strdup(csound, str);
}

PUBLIC char* csoundResolveModulePath(CSOUND *csound, const char *import_path)
{
    char *result = NULL;
    char current_dir[PATH_MAX];
    char test_path[PATH_MAX];
    const char *extension = ".orc";
    char *path_to_use;

    if (import_path == NULL) {
        return NULL;
    }

    MODULE_STATE *state = csoundGetModuleState(csound);

    /* Strip quotes from string paths if present */
    path_to_use = strip_quotes(csound, import_path);
    if (path_to_use == NULL) {
        return NULL;
    }

    /* 1. Check if import_path already has an extension */
    const char *last_dot = strrchr(path_to_use, '.');
    if (last_dot && strcmp(last_dot, ".orc") == 0) {
        extension = "";
    }

    /* 2. Try relative to current file directory */
    if (csound->oparms->infilename != NULL) {
        char *dir_end = strrchr(csound->oparms->infilename, '/');
        if (dir_end != NULL) {
            size_t dir_len = dir_end - csound->oparms->infilename + 1;
            strncpy(current_dir, csound->oparms->infilename, dir_len);
            current_dir[dir_len] = '\0';

            snprintf(test_path, PATH_MAX, "%s%s%s", current_dir, path_to_use, extension);
            if (access(test_path, R_OK) == 0) {
                result = csound->Strdup(csound, test_path);
                csound->Free(csound, path_to_use);
                return result;
            }
        }
    }

    /* 3. Try relative to CSD directory */
    if (csound->csdname != NULL) {
        char *dir_end = strrchr(csound->csdname, '/');
        if (dir_end != NULL) {
            size_t dir_len = dir_end - csound->csdname + 1;
            strncpy(current_dir, csound->csdname, dir_len);
            current_dir[dir_len] = '\0';

            snprintf(test_path, PATH_MAX, "%s%s%s", current_dir, path_to_use, extension);
            if (access(test_path, R_OK) == 0) {
                result = csound->Strdup(csound, test_path);
                csound->Free(csound, path_to_use);
                return result;
            }
        }
    }

    /* 4. Try search paths from csoundAddModuleSearchPath */
    if (state->module_search_paths != NULL) {
        /* This would need hash table iteration - for now, check common paths */
        snprintf(test_path, PATH_MAX, "%s%s%s", "./", path_to_use, extension);
        if (access(test_path, R_OK) == 0) {
            result = csound->Strdup(csound, test_path);
            csound->Free(csound, path_to_use);
            return result;
        }

        snprintf(test_path, PATH_MAX, "%s%s%s", "./lib/", path_to_use, extension);
        if (access(test_path, R_OK) == 0) {
            result = csound->Strdup(csound, test_path);
            csound->Free(csound, path_to_use);
            return result;
        }

        snprintf(test_path, PATH_MAX, "%s%s%s", "./modules/", path_to_use, extension);
        if (access(test_path, R_OK) == 0) {
            result = csound->Strdup(csound, test_path);
            csound->Free(csound, path_to_use);
            return result;
        }
    }

    /* 5. As a fallback, just append extension to the original path */
    size_t len = strlen(path_to_use) + strlen(extension) + 1;
    result = (char*)csound->Malloc(csound, len);
    snprintf(result, len, "%s%s", path_to_use, extension);

    csound->Free(csound, path_to_use);
    return result;
}

PUBLIC CS_MODULE* csoundGetOrLoadModule(CSOUND *csound, const char *path)
{
    CS_MODULE *mod;
    MODULE_STATE *state;

    if (path == NULL) {
        return NULL;
    }

    /* Initialize module system if needed */
    csoundInitModuleSystem(csound);
    state = csoundGetModuleState(csound);

    /* Check cache first - return cached module even if not yet compiled
     * This prevents double-loading during recursive imports */
    mod = (CS_MODULE*)cs_hash_table_get(csound, state->modules, (char*)path);
    if (mod) {
        mod->ref_count++;
        return mod;
    }

    /* Detect circular dependencies */
    for (int i = 0; i < state->module_stack_size; i++) {
        if (strcmp(state->module_stack[i]->normalized_path, path) == 0) {
            csound->Message(csound, "Circular import detected: %s\n", path);
            return NULL;
        }
    }

    /* Push current module onto stack for circular dependency detection */
    if (state->module_stack_size >= state->module_stack_capacity) {
        /* Expand the stack */
        int new_capacity = state->module_stack_capacity * 2;
        CS_MODULE **new_stack = (CS_MODULE**)csound->ReAlloc(csound,
                                   state->module_stack,
                                   sizeof(CS_MODULE*) * new_capacity);
        state->module_stack = new_stack;
        state->module_stack_capacity = new_capacity;
    }

    state->module_stack[state->module_stack_size] = state->current_module;
    state->module_stack_size++;

    /* Load and compile module */
    mod = csoundLoadModuleFromFile(csound, path);
    if (mod) {
        cs_hash_table_put(csound, state->modules, (char*)path, mod);
    }

    /* Pop from stack */
    if (state->module_stack_size > 0) {
        state->module_stack_size--;
    }

    return mod;
}

PUBLIC int csoundAddModuleSearchPath(CSOUND *csound, const char *path)
{
    MODULE_STATE *state = csoundGetModuleState(csound);

    if (state->module_search_paths == NULL) {
        state->module_search_paths = cs_hash_table_create(csound);
    }

    cs_hash_table_put(csound, state->module_search_paths, (char*)path, (void*)1);
    return CSOUND_SUCCESS;
}

PUBLIC CS_VARIABLE* csoundFindModuleVariable(CSOUND *csound,
                                            CS_MODULE *module,
                                            const char *name)
{
    if (module == NULL || name == NULL || module->module_vars == NULL) {
        return NULL;
    }

    return (CS_VARIABLE*)cs_hash_table_get(csound, module->module_vars, (char*)name);
}

PUBLIC OENTRY* csoundFindModuleOpcode(CSOUND *csound,
                                      CS_MODULE *module,
                                      const char *name)
{
    if (module == NULL || name == NULL || module->opcodes == NULL) {
        return NULL;
    }

    return (OENTRY*)cs_hash_table_get(csound, module->opcodes, (char*)name);
}

PUBLIC bool csoundModuleHasItem(CSOUND *csound,
                                CS_MODULE *module,
                                const char *name)
{
    if (module == NULL || name == NULL) {
        return false;
    }

    /* Check variables */
    if (module->module_vars &&
        cs_hash_table_get(csound, module->module_vars, (char*)name) != NULL) {
        return true;
    }

    /* Check opcodes */
    if (module->opcodes &&
        cs_hash_table_get(csound, module->opcodes, (char*)name) != NULL) {
        return true;
    }

    return false;
}

PUBLIC CS_MODULE* csoundCreateVirtualModule(CSOUND *csound, const char *name)
{
    CS_MODULE *mod;

    if (name == NULL) {
        return NULL;
    }

    csoundInitModuleSystem(csound);

    mod = (CS_MODULE*)csound->Calloc(csound, sizeof(CS_MODULE));
    mod->name = csound->Strdup(csound, name);
    mod->normalized_path = csound->Strdup(csound, name);
    mod->is_compiled = true;
    mod->is_virtual = true;
    mod->ref_count = 1;
    mod->opcodes = cs_hash_table_create(csound);

    return mod;
}

PUBLIC int csoundRegisterVirtualOpcode(CSOUND *csound,
                                       CS_MODULE *module,
                                       const char *opname,
                                       int (*init)(CSOUND *, void *),
                                       int (*perf)(CSOUND *, void *),
                                       int (*deinit)(CSOUND *, void *))
{
    OENTRY *op;

    if (module == NULL || opname == NULL || module->opcodes == NULL) {
        return CSOUND_ERROR;
    }

    op = (OENTRY*)csound->Calloc(csound, sizeof(OENTRY));
    op->opname = csound->Strdup(csound, opname);
    op->init = init;
    op->perf = perf;
    op->deinit = deinit;

    cs_hash_table_put(csound, module->opcodes, (char*)opname, op);

    return CSOUND_SUCCESS;
}

/* Internal helper functions */

PUBLIC CS_MODULE* csoundLoadModuleFromFile(CSOUND *csound, const char *path)
{
    CS_MODULE *mod;
    char *full_path;
    FILE *fp;
    char *orc_code = NULL;
    long file_size;
    TREE *ast = NULL;

    /* Resolve the actual file path */
    full_path = csoundResolveModulePath(csound, path);
    if (full_path == NULL) {
        csound->Message(csound, "Could not resolve module path: %s\n", path);
        return NULL;
    }

    /* Check if file exists */
    if (access(full_path, R_OK) != 0) {
        csound->Message(csound, "Module file not found: %s\n", full_path);
        csound->Free(csound, full_path);
        return NULL;
    }

    /* Read the entire file */
    fp = fopen(full_path, "r");
    if (fp == NULL) {
        csound->Message(csound, "Could not open module file: %s\n", full_path);
        csound->Free(csound, full_path);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        csound->Message(csound, "Empty module file: %s\n", full_path);
        fclose(fp);
        csound->Free(csound, full_path);
        return NULL;
    }

    orc_code = (char*)csound->Malloc(csound, file_size + 1);
    if (fread(orc_code, 1, file_size, fp) != (size_t)file_size) {
        csound->Message(csound, "Error reading module file: %s\n", full_path);
        fclose(fp);
        csound->Free(csound, orc_code);
        csound->Free(csound, full_path);
        return NULL;
    }
    orc_code[file_size] = '\0';
    fclose(fp);

    /* Create the module structure BEFORE parsing so current_module is set
     * This allows UDO definitions to capture the correct module varPool */
    mod = (CS_MODULE*)csound->Calloc(csound, sizeof(CS_MODULE));
    mod->file_path = full_path;
    mod->normalized_path = csound->Strdup(csound, path);
    mod->ref_count = 1;
    mod->opcodes = cs_hash_table_create(csound);
    mod->module_vars = cs_hash_table_create(csound);

    /* Create module-level variable pool with parent = engineState->varPool
     * This pool will hold the module's g-variables and @global variables */
    mod->varPool = csoundCreateVarPool(csound);
    mod->varPool->parent = csound->engineState.varPool;

    /* Extract module name from path */
    const char *last_slash = strrchr(path, '/');
    const char *last_dot = strrchr(path, '.');
    if (last_slash && last_dot && last_dot > last_slash) {
        size_t name_len = last_dot - last_slash - 1;
        mod->name = (char*)csound->Malloc(csound, name_len + 1);
        strncpy(mod->name, last_slash + 1, name_len);
        mod->name[name_len] = '\0';
    } else {
        mod->name = csound->Strdup(csound, path);
    }

    /* Set current_module BEFORE parsing so UDO definitions capture the correct module */
    MODULE_STATE *module_state = csoundGetModuleState(csound);
    CS_MODULE *saved_current_module = module_state->current_module;
    module_state->current_module = mod;

    /* Parse the orchestra code into an AST */
    ast = csoundParseOrc(csound, orc_code);

    /* Restore previous current_module */
    module_state->current_module = saved_current_module;

    if (ast == NULL) {
        csound->Message(csound, "Error: Failed to parse module file: %s\n", full_path);
        csound->Free(csound, orc_code);
        csound->Free(csound, full_path);
        csound->Free(csound, mod->name);
        csound->Free(csound, mod);
        return NULL;
    }

    mod->ast = ast;  /* Store the AST */

    /* Mark as NOT yet compiled - will be compiled during import */
    mod->is_compiled = false;

    csound->Free(csound, orc_code);
    return mod;
}

/**
 * Find the index of an imported module in a module's imports array.
 * Returns -1 if not found.
 */
static int find_import_index(CS_MODULE *module, CS_MODULE *imported) {
    if (module == NULL || imported == NULL) {
        return -1;
    }
    for (int32_t i = 0; i < module->import_count; i++) {
        if (module->imports[i] == imported) {
            return i;
        }
    }
    return -1;
}

PUBLIC void csoundProcessImportStatements(CSOUND *csound,
                                         CS_MODULE *module,
                                         TREE *ast)
{
    MODULE_STATE *state = csoundGetModuleState(csound);

    if (module == NULL || ast == NULL) {
        return;
    }

    /* Process import statement based on its type */
    if (ast->type == IMPORT_TOKEN) {
        /* Handle "import module" or "import module as alias" */
        if (ast->left && ast->left->value && ast->left->value->lexeme) {
            char *raw_path = ast->left->value->lexeme;
            char *module_path = strip_quotes(csound, raw_path);

            /* For "import module as alias", use the alias from right child */
            char *alias = NULL;
            if (ast->right && ast->right->value && ast->right->value->lexeme) {
                alias = ast->right->value->lexeme;
            }

            /* Register the entire module as available (wildcard-like) */
            csoundAddModuleImport(csound, state->current_module, module, alias);

            /* Find the import index (may be existing or newly added) */
            int import_idx = find_import_index(state->current_module, module);
            if (import_idx >= 0) {
                /* Create import info with wildcard semantics (all items allowed) */
                CS_IMPORT *import_info = csoundCreateImport(csound,
                    alias ? IMPORT_MODULE_AS : IMPORT_MODULE,
                    module_path, alias, 1);
                state->current_module->import_info[import_idx] = import_info;
            }

            if (csound->GetDebug(csound) > 99) {
                csound->Message(csound, "Processed import: %s%s%s\n",
                               module_path,
                               alias ? " as " : "",
                               alias ? alias : "");
            }

            csound->Free(csound, module_path);
        }
    } else if (ast->type == FROM_TOKEN) {
        /* Handle "from module import item1, item2" or "from module import *" */
        if (ast->left && ast->left->value && ast->left->value->lexeme) {
            char *raw_path = ast->left->value->lexeme;
            char *module_path = strip_quotes(csound, raw_path);

            /* Process the import list from the right child */
            if (ast->right) {
                TREE *import_list = ast->right;

                /* Check if this is a wildcard import */
                if (import_list->type == '*') {
                    /* Register the module with wildcard semantics */
                    csoundAddModuleImport(csound, state->current_module, module, NULL);

                    /* Find the import index and create import info with wildcard */
                    int import_idx = find_import_index(state->current_module, module);
                    if (import_idx >= 0) {
                        CS_IMPORT *import_info = csoundCreateImport(csound, IMPORT_FROM_ALL, module_path, NULL, 1);
                        state->current_module->import_info[import_idx] = import_info;
                    }

                    csoundRegisterModuleItems(csound, module);

                    if (csound->GetDebug(csound) > 99) {
                        csound->Message(csound, "Processed wildcard import: from %s import *\n", module_path);
                    }
                } else {
                    /* Add the module to imports first */
                    csoundAddModuleImport(csound, state->current_module, module, NULL);

                    /* Find the import index and create import info with selective items */
                    int import_idx = find_import_index(state->current_module, module);
                    CS_IMPORT *import_info = NULL;
                    if (import_idx >= 0) {
                        import_info = csoundCreateImport(csound, IMPORT_FROM, module_path, NULL, 0);
                        state->current_module->import_info[import_idx] = import_info;
                    }

                    /* Process specific item imports */
                    TREE *current_item = import_list;
                    while (current_item) {
                        if (current_item->value && current_item->value->lexeme) {
                            char *item_name = current_item->value->lexeme;

                            /* Check if this item has an alias */
                            char *alias = NULL;
                            if (current_item->right && current_item->right->value &&
                                current_item->right->value->lexeme) {
                                alias = current_item->right->value->lexeme;
                            }

                            /* Add to import info's item list */
                            if (import_info != NULL) {
                                csoundAddImportItem(csound, import_info, item_name, alias);
                            }

                            /* Register this specific item from the module */
                            csoundRegisterModuleItem(csound, state->current_module, module,
                                                    item_name, alias);

                            if (csound->GetDebug(csound) > 99) {
                                csound->Message(csound, "Processed item import: %s%s%s from %s\n",
                                               item_name,
                                               alias ? " as " : "",
                                               alias ? alias : "",
                                               module_path);
                            }
                        }
                        current_item = current_item->next;
                    }
                }
            }

            csound->Free(csound, module_path);
        }
    }
}

PUBLIC void csoundRegisterModuleItems(CSOUND *csound, CS_MODULE *module)
{
    MODULE_STATE *state = csoundGetModuleState(csound);

    if (module == NULL) {
        return;
    }

    /* Module items (UDOs and variables) are now registered automatically
     * during csound_compile_tree() when the module's AST is compiled.
     * No manual parsing or registration needed. */

    if (csound->GetDebug(csound) > 99) {
        csound->Message(csound, "Module items will be registered during AST compilation: %s\n",
                       module->name ? module->name : "unknown");
    }

    (void)state; /* Suppress unused warning */
}

PUBLIC CS_MODULE* csoundFindModuleByAlias(CSOUND *csound, const char *alias)
{
    MODULE_STATE *state = csoundGetModuleState(csound);
    if (state == NULL || state->import_aliases == NULL || alias == NULL) {
        return NULL;
    }
    return (CS_MODULE*)cs_hash_table_get(csound, state->import_aliases, (char*)alias);
}

PUBLIC CS_MODULE* csoundFindImportedModule(CSOUND *csound,
                                          CS_MODULE *current,
                                          const char *name)
{
    if (current == NULL || name == NULL) {
        return NULL;
    }

    /* First, check global import_aliases for "import X as alias" syntax */
    CS_MODULE *aliased = csoundFindModuleByAlias(csound, name);
    if (aliased != NULL) {
        return aliased;
    }

    /* Search current module's imports by module name */
    for (int32_t i = 0; i < current->import_count; i++) {
        CS_MODULE *imported = current->imports[i];
        if (imported != NULL && imported->name != NULL) {
            if (strcmp(imported->name, name) == 0) {
                return imported;
            }
        }
    }

    return NULL;
}

PUBLIC int csoundAddModuleImport(CSOUND *csound,
                                CS_MODULE *module,
                                CS_MODULE *imported,
                                const char *alias)
{
    if (module == NULL || imported == NULL) {
        return CSOUND_ERROR;
    }

    /* Check if this module is already imported (avoid duplicates) */
    int existing_idx = find_import_index(module, imported);
    if (existing_idx >= 0) {
        /* Already imported - just update alias if provided */
        if (alias != NULL && strlen(alias) > 0) {
            MODULE_STATE *state = csoundGetModuleState(csound);
            if (state->import_aliases != NULL) {
                cs_hash_table_put(csound, state->import_aliases, (char*)alias, imported);
            }
        }
        return CSOUND_SUCCESS;  /* Already imported, no need to add again */
    }

    /* Check if we need to expand the imports array */
    if (module->import_count >= module->import_capacity) {
        int32_t new_capacity = module->import_capacity == 0 ? 4 : module->import_capacity * 2;
        CS_MODULE **new_imports = csound->ReAlloc(csound, module->imports,
                                                  new_capacity * sizeof(CS_MODULE*));
        CS_IMPORT **new_import_info = csound->ReAlloc(csound, module->import_info,
                                                       new_capacity * sizeof(CS_IMPORT*));
        if (new_imports == NULL || new_import_info == NULL) {
            return CSOUND_ERROR;
        }
        module->imports = new_imports;
        module->import_info = new_import_info;
        module->import_capacity = new_capacity;
    }

    /* Add the imported module to the imports array */
    module->imports[module->import_count] = imported;
    module->import_info[module->import_count] = NULL;  /* Will be set by caller if needed */
    module->import_count++;

    /* Store alias→module mapping if alias is provided */
    if (alias != NULL && strlen(alias) > 0) {
        MODULE_STATE *state = csoundGetModuleState(csound);
        if (state->import_aliases != NULL) {
            cs_hash_table_put(csound, state->import_aliases, (char*)alias, imported);
        }
    }

    return CSOUND_SUCCESS;
}

PUBLIC void csoundRegisterModuleItem(CSOUND *csound,
                                    CS_MODULE *target_module,
                                    CS_MODULE *source_module,
                                    const char *item_name,
                                    const char *alias)
{
    /* Register a specific item from source_module to target_module */
    if (source_module == NULL || item_name == NULL) {
        return;
    }

    /* Check if the item is a variable */
    CS_VARIABLE *var = csoundFindModuleVariable(csound, source_module, item_name);
    if (var) {
        /* For now, we'll make it globally available */
        /* In a full implementation, this would add to target_module's namespace */
        if (csound->GetDebug(csound) > 99) {
            csound->Message(csound, "Registered variable: %s%s%s from module %s\n",
                           item_name,
                           alias ? " as " : "",
                           alias ? alias : "",
                           source_module->name ? source_module->name : "unknown");
        }
        return;
    }

    /* Check if the item is an opcode */
    OENTRY *opcode = csoundFindModuleOpcode(csound, source_module, item_name);
    if (opcode) {
        /* For now, we'll make it globally available */
        /* In a full implementation, this would add to target_module's namespace */
        if (csound->GetDebug(csound) > 99) {
            csound->Message(csound, "Registered opcode: %s%s%s from module %s\n",
                           item_name,
                           alias ? " as " : "",
                           alias ? alias : "",
                           source_module->name ? source_module->name : "unknown");
        }
        return;
    }

    /* Item not found in module */
    if (csound->GetDebug(csound) > 99) {
        csound->Message(csound, "Warning: Item '%s' not found in module %s\n",
                       item_name, source_module->name ? source_module->name : "unknown");
    }

    (void)target_module; /* Suppress unused warning */
}

PUBLIC CS_IMPORT* csoundCreateImport(CSOUND *csound,
                                     IMPORT_TYPE type,
                                     const char *module_path,
                                     const char *alias,
                                     int is_wildcard)
{
    CS_IMPORT *import = csound->Calloc(csound, sizeof(CS_IMPORT));
    import->type = type;
    import->module_path = module_path ? csoundStrdup(csound, module_path) : NULL;
    import->module_alias = alias ? csoundStrdup(csound, alias) : NULL;
    import->is_wildcard = is_wildcard;
    import->items = NULL;
    import->item_count = 0;
    return import;
}

PUBLIC void csoundAddImportItem(CSOUND *csound,
                                CS_IMPORT *import_info,
                                const char *original_name,
                                const char *local_name)
{
    if (import_info == NULL || original_name == NULL) {
        return;
    }

    /* Expand items array */
    int new_count = import_info->item_count + 1;
    CS_IMPORT_ITEM *new_items = csound->ReAlloc(csound, import_info->items,
                                                 new_count * sizeof(CS_IMPORT_ITEM));
    if (new_items == NULL) {
        return;
    }
    import_info->items = new_items;

    /* Add new item */
    CS_IMPORT_ITEM *item = &import_info->items[import_info->item_count];
    item->original_name = csoundStrdup(csound, original_name);
    item->local_name = local_name ? csoundStrdup(csound, local_name) : csoundStrdup(csound, original_name);
    item->is_wildcard = false;

    import_info->item_count = new_count;
}

PUBLIC int csoundIsItemImportAllowed(CSOUND *csound,
                                     CS_MODULE *importing_module,
                                     CS_MODULE *source_module,
                                     const char *item_name)
{
    if (importing_module == NULL || source_module == NULL || item_name == NULL) {
        return 1;  /* Default to allow if no info available */
    }

    /* Search for the import info for this source module */
    for (int i = 0; i < importing_module->import_count; i++) {
        if (importing_module->imports[i] == source_module) {
            CS_IMPORT *info = importing_module->import_info ? importing_module->import_info[i] : NULL;

            /* If no import info, this was a plain "import" - all items allowed */
            if (info == NULL) {
                return 1;
            }

            /* Wildcard import allows everything */
            if (info->is_wildcard) {
                return 1;
            }

            /* Check if item is in the explicit list */
            for (int j = 0; j < info->item_count; j++) {
                if (strcmp(info->items[j].original_name, item_name) == 0) {
                    return 1;
                }
            }

            /* Item not in explicit list */
            return 0;
        }
    }

    /* Module not found in imports - shouldn't happen, but allow */
    return 1;
}
