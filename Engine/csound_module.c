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
    root->name = cs_strdup(csound, "<root>");
    root->file_path = NULL;
    root->normalized_path = cs_strdup(csound, "<root>");

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

            /* Register the entire module as available */
            csoundAddModuleImport(csound, state->current_module, module, alias);

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
                    /* Register all items from module */
                    csoundRegisterModuleItems(csound, module);

                    if (csound->GetDebug(csound) > 99) {
                        csound->Message(csound, "Processed wildcard import: from %s import *\n", module_path);
                    }
                } else {
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

PUBLIC CS_MODULE* csoundFindImportedModule(CSOUND *csound,
                                          CS_MODULE *current,
                                          const char *name)
{
    /* TODO: Search imported modules by name or alias */
    (void)csound; (void)current; (void)name;
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

    /* Check if we need to expand the imports array */
    if (module->import_count >= module->import_capacity) {
        int32_t new_capacity = module->import_capacity == 0 ? 4 : module->import_capacity * 2;
        CS_MODULE **new_imports = csound->ReAlloc(csound, module->imports,
                                                  new_capacity * sizeof(CS_MODULE*));
        if (new_imports == NULL) {
            return CSOUND_ERROR;
        }
        module->imports = new_imports;
        module->import_capacity = new_capacity;
    }

    /* Add the imported module to the imports array */
    module->imports[module->import_count] = imported;
    module->import_count++;

    csound->Message(csound, "Added module import: %s imports %s (count=%d)\n",
                    module->name ? module->name : "(root)",
                    imported->name ? imported->name : "(unnamed)",
                    module->import_count);

    (void)alias; /* TODO: Store alias for module-level namespace resolution */
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

