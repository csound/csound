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
#include "csound_standard_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#include <io.h>
#define access _access
#ifndef R_OK
#define R_OK 4
#endif
#else
#include <unistd.h>
#endif

MODULE_STATE *csoundGetModuleState(CSOUND *csound)
{
    if (csound->module_state == NULL)
        csound->module_state = csound->Calloc(csound, sizeof(MODULE_STATE));
    return csound->module_state;
}

static void initialize_module_system(CSOUND *csound)
{
    MODULE_STATE *state = csoundGetModuleState(csound);
    if (state->modules == NULL)
        state->modules = cs_hash_table_create(csound);
}

static CS_MODULE *create_module(CSOUND *csound, const char *name)
{
    MODULE_STATE *state = csoundGetModuleState(csound);
    CS_MODULE *module = csound->Calloc(csound, sizeof(*module));
    module->name = csoundStrdup(csound, name);
    module->id = state->next_module_id++;
    module->opcodes = cs_hash_table_create(csound);
    module->aliases = cs_hash_table_create(csound);
    module->types = cs_hash_table_create(csound);
    module->varPool = csoundCreateVarPool(csound);
    return module;
}

static char *canonical_path(CSOUND *csound, const char *path);

CS_MODULE *csoundCreateRootModule(CSOUND *csound)
{
    initialize_module_system(csound);
    MODULE_STATE *state = csoundGetModuleState(csound);
    if (state->root_module == NULL) {
        state->root_module = create_module(csound, "<root>");
        csoundFreeVarPool(csound, state->root_module->varPool);
        state->root_module->varPool = csound->engineState.varPool;
    }
    if (state->current_module == NULL || state->current_module == state->root_module) {
        const char *source = csound->csdname != NULL ? csound->csdname : csound->orchname;
        if (source != NULL) {
            char *file = canonical_path(csound, source);
            if (file != NULL) {
                if (state->root_module->file_path == NULL ||
                    strcmp(state->root_module->file_path, file) != 0)
                    state->root_module->file_path = file;
                else csound->Free(csound, file);
            }
        }
        state->current_module = state->root_module;
    }
    return state->root_module;
}

static CS_HASH_TABLE *copy_bindings(CSOUND *csound, CS_HASH_TABLE *source)
{
    CS_HASH_TABLE *copy = cs_hash_table_create(csound);
    for (int32_t i = 0; i < source->table_size; i++)
        for (CS_HASH_TABLE_ITEM *item = source->buckets[i]; item; item = item->next)
            cs_hash_table_put(csound, copy, item->key, item->value);
    return copy;
}

CS_MODULE_UPDATE *csoundBeginModuleUpdate(CSOUND *csound)
{
    MODULE_STATE *state = csoundGetModuleState(csound);
    CS_MODULE *module = state->current_module;
    if (module != state->root_module) return NULL;
    CS_MODULE_UPDATE *update = csound->Calloc(csound, sizeof(*update));
    CS_MODULE *scope = csound->Calloc(csound, sizeof(*scope));
    *scope = *module;
    scope->owner = module;
    scope->id = state->next_module_id++;
    scope->varPool = csoundCreateVarPool(csound);
    scope->varPool->parent = module->compiler_scope != NULL &&
                            !module->compiler_scope->is_compiled
        ? module->compiler_scope->varPool : csound->engineState.varPool;
    scope->opcodes = copy_bindings(csound, module->opcodes);
    scope->aliases = copy_bindings(csound, module->aliases);
    scope->types = copy_bindings(csound, module->types);
    size_t count = module->import_count;
    scope->imports = csound->Malloc(csound, (count + 1) * sizeof(CS_MODULE *));
    scope->import_info = csound->Malloc(csound, (count + 1) * sizeof(CS_IMPORT *));
    if (count != 0) {
        memcpy(scope->imports, module->imports, count * sizeof(CS_MODULE *));
        memcpy(scope->import_info, module->import_info, count * sizeof(CS_IMPORT *));
    }
    update->module = module;
    update->scope = scope;
    update->previous_scope = module->compiler_scope;
    update->opcodes = module->opcodes;
    update->aliases = module->aliases;
    update->types = module->types;
    update->imports = module->imports;
    update->import_info = module->import_info;
    update->import_count = module->import_count;
    state->current_module = scope;
    return update;
}

/* Accepted async edits must be visible to the next compiler request before merge. */
void csoundAcceptModuleUpdate(CS_MODULE_UPDATE *update)
{
    if (update == NULL) return;
    CS_MODULE *module = update->module, *scope = update->scope;
    module->opcodes = scope->opcodes;
    module->aliases = scope->aliases;
    module->types = scope->types;
    module->imports = scope->imports;
    module->import_info = scope->import_info;
    module->import_count = scope->import_count;
    module->compiler_scope = scope;
    update->accepted = true;
}

void csoundFinishModuleUpdate(CSOUND *csound, CS_MODULE_UPDATE *update, int commit)
{
    if (update == NULL) return;
    CS_MODULE *module = update->module;
    if (!commit && update->accepted && module->compiler_scope == update->scope) {
        module->opcodes = update->opcodes;
        module->aliases = update->aliases;
        module->types = update->types;
        module->imports = update->imports;
        module->import_info = update->import_info;
        module->import_count = update->import_count;
        module->compiler_scope = update->previous_scope;
    }
    if (commit) {
        update->scope->is_compiled = true;
        update->scope->varPool->parent = csound->engineState.varPool;
    }
    /* Namespace versions can still be held by parsed trees or queued edits. */
    csound->Free(csound, update);
}

static char *unquote(CSOUND *csound, const char *path)
{
    size_t length = strlen(path);
    if (length >= 2 && path[0] == '"' && path[length - 1] == '"')
        return cs_strndup(csound, path + 1, length - 2);
    return csoundStrdup(csound, path);
}

static char *canonical_path(CSOUND *csound, const char *path)
{
    char resolved[PATH_MAX];
    if (access(path, R_OK) != 0) return NULL;
#if defined(WIN32) && !defined(__CYGWIN__)
    if (_fullpath(resolved, path, sizeof(resolved)) == NULL) return NULL;
#else
    if (realpath(path, resolved) == NULL) return NULL;
#endif
    return csoundStrdup(csound, resolved);
}

static char *relative_path(CSOUND *csound, const char *file, const char *path)
{
    if (file == NULL) return NULL;
    const char *slash = strrchr(file, '/');
#if defined(WIN32) && !defined(__CYGWIN__)
    const char *backslash = strrchr(file, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
#endif
    if (slash == NULL) return NULL;
    size_t prefix = slash - file + 1;
    char *candidate = csound->Malloc(csound, prefix + strlen(path) + 1);
    memcpy(candidate, file, prefix);
    strcpy(candidate + prefix, path);
    char *result = canonical_path(csound, candidate);
    csound->Free(csound, candidate);
    return result;
}

static char *resolve_module_path(CSOUND *csound, const char *path)
{
    if (path == NULL) return NULL;
    MODULE_STATE *state = csoundGetModuleState(csound);
    char *name = unquote(csound, path);
    size_t length = strlen(name);
    if (length < 4 || strcmp(name + length - 4, ".orc") != 0) {
        name = csound->ReAlloc(csound, name, length + 5);
        strcpy(name + length, ".orc");
    }
    char *result = NULL;
    int absolute = name[0] == '/' || name[0] == '\\' ||
                   (strlen(name) > 1 && name[1] == ':');
    if (absolute) result = canonical_path(csound, name);
    else {
        if (state->current_module != NULL)
            result = relative_path(csound, state->current_module->file_path, name);
        if (result == NULL) result = relative_path(csound, csound->csdname, name);
        for (int32_t i = 0; result == NULL && i < state->search_path_count; i++) {
            size_t size = strlen(state->search_paths[i]) + strlen(name) + 2;
            char *candidate = csound->Malloc(csound, size);
            snprintf(candidate, size, "%s/%s", state->search_paths[i], name);
            result = canonical_path(csound, candidate);
            csound->Free(csound, candidate);
        }
        const char *fallbacks[] = {"./", "./lib/", "./modules/"};
        for (int i = 0; result == NULL && i < 3; i++) {
            size_t size = strlen(fallbacks[i]) + strlen(name) + 1;
            char *candidate = csound->Malloc(csound, size);
            snprintf(candidate, size, "%s%s", fallbacks[i], name);
            result = canonical_path(csound, candidate);
            csound->Free(csound, candidate);
        }
    }
    csound->Free(csound, name);
    return result;
}

PUBLIC int32_t csoundAddModuleSearchPath(CSOUND *csound, const char *path)
{
    if (path == NULL || *path == '\0') return CSOUND_ERROR;
    MODULE_STATE *state = csoundGetModuleState(csound);
    for (int32_t i = 0; i < state->search_path_count; i++)
        if (strcmp(state->search_paths[i], path) == 0) return CSOUND_SUCCESS;
    state->search_paths = csound->ReAlloc(csound, state->search_paths,
                           (state->search_path_count + 1) * sizeof(char *));
    state->search_paths[state->search_path_count++] = csoundStrdup(csound, path);
    return CSOUND_SUCCESS;
}

CS_MODULE *csoundGetOrLoadModule(CSOUND *csound, const char *path)
{
    initialize_module_system(csound);
    MODULE_STATE *state = csoundGetModuleState(csound);
    char *file = resolve_module_path(csound, path);
    if (file == NULL) {
        synterr(csound, Str("Module file not found: %s\n"), path);
        return NULL;
    }
    CS_MODULE *module = cs_hash_table_get(csound, state->modules, file);
    if (module != NULL) {
        csound->Free(csound, file);
        if (module->is_loading) {
            synterr(csound, Str("Circular import detected: %s\n"), module->file_path);
            return NULL;
        }
        if (module->ast != NULL) return module;
    } else {
        const char *base = strrchr(file, '/');
        const char *backslash = strrchr(file, '\\');
        if (backslash != NULL && (base == NULL || backslash > base)) base = backslash;
        base = base != NULL ? base + 1 : file;
        module = create_module(csound, base);
        char *extension = strrchr(module->name, '.');
        if (extension != NULL) *extension = '\0';
        module->file_path = file;
        cs_hash_table_put(csound, state->modules, file, module);
    }
    FILE *fp = fopen(module->file_path, "rb");
    char *code = NULL;
    if (fp == NULL || fseek(fp, 0, SEEK_END) != 0) goto read_failed;
    long size = ftell(fp);
    if (size < 0 || fseek(fp, 0, SEEK_SET) != 0) goto read_failed;
    code = csound->Malloc(csound, (size_t)size + 1);
    size_t read = fread(code, 1, (size_t)size, fp);
    if (ferror(fp) || read != (size_t)size) goto read_failed;
    fclose(fp);
    code[size] = '\0';
    CS_MODULE *previous = state->current_module;
    module->is_loading = true;
    state->current_module = module;
    module->ast = csoundParseOrc(csound, code);
    state->current_module = previous;
    module->is_loading = false;
    csound->Free(csound, code);
    if (module->ast == NULL) {
        cs_hash_table_remove(csound, state->modules, module->file_path);
        return NULL;
    }
    return module;
 read_failed:
    if (fp != NULL) fclose(fp);
    csound->Free(csound, code);
    cs_hash_table_remove(csound, state->modules, module->file_path);
    synterr(csound, Str("Could not read module file: %s\n"), module->file_path);
    return NULL;
}

CS_VARIABLE *csoundFindModuleVariable(CSOUND *csound, CS_MODULE *module,
                                     const char *name)
{
    if (module == NULL || name == NULL) return NULL;
    CS_VARIABLE *var = cs_hash_table_get(csound, module->varPool->table, (char *)name);
    if (var == NULL && module->varPool->bindings != NULL)
        var = cs_hash_table_get(csound, module->varPool->bindings, (char *)name);
    return var;
}

CS_VARIABLE *csoundFindSystemVariable(CSOUND *csound, const char *name)
{
    CS_VARIABLE *var = csoundFindVariableWithName(csound, csound->engineState.varPool, name);
    if (var != NULL && (var->varType == &CS_VAR_TYPE_R ||
                        var->varType == &CS_VAR_TYPE_OPCODEREF ||
                        strcmp(name, "true") == 0 || strcmp(name, "false") == 0 ||
                        strcmp(name, "truek") == 0 || strcmp(name, "falsek") == 0))
        return var;
    return NULL;
}

CONS_CELL *csoundFindModuleOpcodes(CSOUND *csound, CS_MODULE *module,
                                  const char *name)
{
    return module != NULL ? cs_hash_table_get(csound, module->opcodes, (char *)name) : NULL;
}

CS_MODULE *csoundFindModuleByAlias(CSOUND *csound, const char *alias)
{
    CS_MODULE *module = csoundGetModuleState(csound)->current_module;
    return module != NULL ? cs_hash_table_get(csound, module->aliases, (char *)alias) : NULL;
}

char *csoundModuleInstrumentName(CSOUND *csound, CS_MODULE *module, const char *name)
{
    if (module == NULL || csoundModuleOwner(module) == csoundGetModuleState(csound)->root_module)
        return csoundStrdup(csound, name);
    size_t size = strlen(name) + 32;
    char *result = csound->Malloc(csound, size);
    snprintf(result, size, "#module_%d_%s", module->id, name);
    return result;
}

static int bind_item(CSOUND *csound, CS_MODULE *target, CS_MODULE *source,
                     CS_VAR_POOL *pool, const char *name, const char *local)
{
    CS_VARIABLE *var = csoundFindModuleVariable(csound, source, name);
    CONS_CELL *opcodes = csoundFindModuleOpcodes(csound, source, name);
    void *type = cs_hash_table_get(csound, source->types, (char *)name);
    if (var == NULL && opcodes == NULL && type == NULL) {
        synterr(csound, Str("Item '%s' not found in module '%s'\n"), name, source->file_path);
        return CSOUND_ERROR;
    }
    if (var != NULL) {
        if (pool->bindings == NULL) pool->bindings = cs_hash_table_create(csound);
        cs_hash_table_put(csound, pool->bindings, (char *)local, var);
    }
    if (opcodes != NULL) cs_hash_table_put(csound, target->opcodes, (char *)local, opcodes);
    if (type != NULL) cs_hash_table_put(csound, target->types, (char *)local, type);
    return CSOUND_SUCCESS;
}

int csoundProcessImportStatements(CSOUND *csound, CS_MODULE *source,
                                  TREE *tree, CS_VAR_POOL *pool)
{
    CS_MODULE *target = csoundGetModuleState(csound)->current_module;
    if (target == NULL || source == NULL) return CSOUND_ERROR;
    tree->markup = source;
    CS_IMPORT *info = csound->Calloc(csound, sizeof(*info));
    info->is_wildcard = tree->type == IMPORT_TOKEN ||
        (tree->right != NULL && tree->right->value != NULL &&
         strcmp(tree->right->value->lexeme, "*") == 0);
    int32_t count = target->import_count++;
    target->imports = csound->ReAlloc(csound, target->imports,
                                      (count + 1) * sizeof(CS_MODULE *));
    target->import_info = csound->ReAlloc(csound, target->import_info,
                                          (count + 1) * sizeof(CS_IMPORT *));
    target->imports[count] = source;
    target->import_info[count] = info;
    if (tree->type == IMPORT_TOKEN) {
        const char *alias = tree->right != NULL ? tree->right->value->lexeme : source->name;
        cs_hash_table_put(csound, target->aliases, (char *)alias, source);
    }
    if (info->is_wildcard) {
        CS_HASH_TABLE *tables[] = {source->varPool->table, source->varPool->bindings,
                                  source->opcodes, source->types};
        for (int t = 0; t < 4; t++) {
            CS_HASH_TABLE *table = tables[t];
            if (table == NULL) continue;
            for (int32_t i = 0; i < table->table_size; i++)
                for (CS_HASH_TABLE_ITEM *item = table->buckets[i]; item; item = item->next)
                    bind_item(csound, target, source, pool, item->key, item->key);
        }
    } else {
        for (TREE *item = tree->right; item != NULL; item = item->next) {
            const char *name = item->value->lexeme;
            const char *local = item->right != NULL ? item->right->value->lexeme : name;
            info->items = csound->ReAlloc(csound, info->items,
                                (info->item_count + 1) * sizeof(CS_IMPORT_ITEM));
            CS_IMPORT_ITEM *binding = &info->items[info->item_count++];
            binding->original_name = csoundStrdup(csound, name);
            binding->local_name = csoundStrdup(csound, local);
            if (bind_item(csound, target, source, pool, name, local) != CSOUND_SUCCESS)
                return CSOUND_ERROR;
        }
    }
    return CSOUND_SUCCESS;
}

int csoundIsItemImportAllowed(CSOUND *csound, CS_MODULE *target,
                              CS_MODULE *source, const char *name)
{
    IGN(csound);
    if (target == NULL || source == NULL) return 0;
    for (int32_t i = 0; i < target->import_count; i++) {
        if (target->imports[i] != source) continue;
        CS_IMPORT *info = target->import_info[i];
        if (info->is_wildcard) return 1;
        for (int32_t j = 0; j < info->item_count; j++)
            if (strcmp(info->items[j].original_name, name) == 0) return 1;
    }
    return 0;
}
