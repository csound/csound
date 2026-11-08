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
#include "environ.h"
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

/* Public structs retain their develop layout; only module-aware objects need
   these private records. Pointer keys are copied by the existing hash table. */
static void pointer_key(char *key, size_t size, const void *pointer)
{
    snprintf(key, size, "%p", pointer);
}

CS_HASH_TABLE *csoundModuleBindings(CSOUND *csound, const CS_VAR_POOL *pool, int create)
{
    MODULE_STATE *state = csound->module_state;
    if (state == NULL && !create) return NULL;
    if (state == NULL) state = csoundGetModuleState(csound);
    if (state->pool_bindings == NULL) {
        if (!create) return NULL;
        state->pool_bindings = cs_hash_table_create(csound);
    }
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), pool);
    CS_HASH_TABLE *bindings = cs_hash_table_get(csound, state->pool_bindings, key);
    if (bindings == NULL && create) {
        bindings = cs_hash_table_create(csound);
        cs_hash_table_put(csound, state->pool_bindings, key, bindings);
    }
    return bindings;
}

void csoundFreeModuleBindings(CSOUND *csound, CS_VAR_POOL *pool)
{
    CS_HASH_TABLE *bindings = csoundModuleBindings(csound, pool, 0);
    if (bindings == NULL) return;
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), pool);
    cs_hash_table_remove(csound, csound->module_state->pool_bindings, key);
    cs_hash_table_free(csound, bindings);
}

CS_MODULE_INSTRUMENT *csoundModuleInstrument(CSOUND *csound, const INSTRTXT *instrument)
{
    MODULE_STATE *state = csoundGetModuleState(csound);
    if (state->instruments == NULL) state->instruments = cs_hash_table_create(csound);
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), instrument);
    CS_MODULE_INSTRUMENT *scope = cs_hash_table_get(csound, state->instruments, key);
    if (scope == NULL) {
        scope = csound->Calloc(csound, sizeof(*scope));
        cs_hash_table_put(csound, state->instruments, key, scope);
    }
    return scope;
}

void csoundFreeModuleInstrument(CSOUND *csound, const INSTRTXT *instrument)
{
    MODULE_STATE *state = csound->module_state;
    if (state == NULL || state->instruments == NULL) return;
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), instrument);
    CS_MODULE_INSTRUMENT *scope = cs_hash_table_get(csound, state->instruments, key);
    if (scope == NULL) return;
    cs_hash_table_remove(csound, state->instruments, key);
    csound->Free(csound, scope->dependencies);
    csound->Free(csound, scope);
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
    module->type_bindings = cs_hash_table_create(csound);
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
    scope->type_bindings = copy_bindings(csound, module->type_bindings);
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
    update->type_bindings = module->type_bindings;
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
    module->type_bindings = scope->type_bindings;
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
        module->type_bindings = update->type_bindings;
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
        char **paths = csoundGetSearchPathFromEnv(csound, "INCDIR");
        for (int32_t i = 0; result == NULL && paths != NULL && paths[i] != NULL; ++i) {
            size_t size = strlen(paths[i]) + strlen(name) + 2;
            char *candidate = csound->Malloc(csound, size);
            snprintf(candidate, size, "%s/%s", paths[i], name);
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
    CS_HASH_TABLE *bindings = csoundModuleBindings(csound, module->varPool, 0);
    if (var == NULL && bindings != NULL)
        var = cs_hash_table_get(csound, bindings, (char *)name);
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

const CS_TYPE *csoundFindModuleType(CSOUND *csound, CS_MODULE *module,
                                   const char *name)
{
    if (module == NULL) return NULL;
    const CS_TYPE *type = cs_hash_table_get(csound, module->types, (char *)name);
    return type != NULL ? type
        : cs_hash_table_get(csound, module->type_bindings, (char *)name);
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
    const CS_TYPE *type = csoundFindModuleType(csound, source, name);
    if (var == NULL && opcodes == NULL && type == NULL) {
        synterr(csound, Str("Item '%s' not found in module '%s'\n"), name, source->file_path);
        return CSOUND_ERROR;
    }
    if (var != NULL) {
        cs_hash_table_put(csound, csoundModuleBindings(csound, pool, 1),
                          (char *)local, var);
    }
    if (opcodes != NULL) cs_hash_table_put(csound, target->opcodes, (char *)local, opcodes);
    if (type != NULL)
        cs_hash_table_put(csound, target->type_bindings, (char *)local, (void *)type);
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
        CS_HASH_TABLE *tables[] = {source->varPool->table, csoundModuleBindings(csound, source->varPool, 0),
                                  source->opcodes, source->types, source->type_bindings};
        for (int t = 0; t < 5; t++) {
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

const char *csoundModuleTypeSignature(CSOUND *csound, const CS_TYPE *type)
{
    if (type == NULL) return NULL;
    MODULE_STATE *state = csound->module_state;
    if (type->userDefinedType != CS_TYPE_USER_DEFINED || state == NULL ||
        state->type_signatures == NULL) return type->varTypeName;
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), type);
    const char *signature = cs_hash_table_get(csound, state->type_signatures, key);
    return signature != NULL ? signature : type->varTypeName;
}

int csoundRegisterModuleType(CSOUND *csound, CS_MODULE *module,
                             const char *name, const CS_TYPE *type)
{
    MODULE_STATE *state = csoundGetModuleState(csound);
    if (state->type_names == NULL) {
        state->type_names = cs_hash_table_create(csound);
        state->type_signatures = cs_hash_table_create(csound);
    }
    size_t size = strlen(name) + 32;
    char *signature = csound->Malloc(csound, size);
    snprintf(signature, size, ":#module_%d_%s;", module->id, name);
    if (cs_hash_table_get(csound, state->type_names, signature) != NULL) {
        csound->Free(csound, signature);
        return 0;
    }
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), type);
    cs_hash_table_put(csound, state->type_signatures, key, signature);
    cs_hash_table_put(csound, state->type_names, signature, (void *)type);
    cs_hash_table_put(csound, module->types, (char *)name, (void *)type);
    return 1;
}

void csoundPublishModuleTypes(CSOUND *csound, CS_MODULE *scope)
{
    if (csoundModuleOwner(scope) != csoundGetModuleState(csound)->root_module) return;
    for (int i = 0; i < scope->types->table_size; ++i) {
        for (CS_HASH_TABLE_ITEM *item = scope->types->buckets[i]; item; item = item->next) {
            const CS_TYPE *type = item->value;
            CS_TYPE_ITEM *slot = csound->typePool->head;
            while (slot != NULL && strcmp(slot->cstype->varTypeName, type->varTypeName))
                slot = slot->next;
            if (slot != NULL) slot->cstype = type;
            else csoundAddVariableType(csound, csound->typePool, type);
        }
    }
}

const CS_TYPE *csoundResolveModuleType(CSOUND *csound, const char *name)
{
    if (name == NULL || *name == '\0') return NULL;
    MODULE_STATE *state = csound->module_state;
    if (state == NULL) return csoundGetTypeWithVarTypeName(csound->typePool, name);
    int canonical = !strncmp(name, ":#module_", 9) || !strncmp(name, "#module_", 8);
    if (canonical && state->type_names != NULL) {
        char *quoted = NULL;
        const char *key = name;
        if (*name != ':') {
            quoted = csound->Malloc(csound, strlen(name) + 3);
            sprintf(quoted, ":%s;", name);
            key = quoted;
        }
        const CS_TYPE *type = cs_hash_table_get(csound, state->type_names, (char *)key);
        csound->Free(csound, quoted);
        return type;
    }
    CS_MODULE *module = state->current_module;
    const CS_TYPE *global = csoundGetTypeWithVarTypeName(csound->typePool, name);
    /* Builtins retain their direct spelling, including one-letter types. */
    if (global != NULL && (global->userDefinedType != CS_TYPE_USER_DEFINED ||
        csoundModuleTypeSignature(csound, global) == global->varTypeName)) return global;
    if (module == NULL) return global;
    size_t len = strlen(name);
    const char *start = name;
    if (len > 2 && name[0] == ':' && name[len - 1] == ';') { ++start; len -= 2; }
    char *plain = cs_strndup(csound, start, len);
    const CS_TYPE *type = csoundFindModuleType(csound, module, plain);
    csound->Free(csound, plain);
    return type;
}

/* Translate only quoted descriptors; array rank and scalar rate tokens remain
   unchanged. Public lookup uses the published root registry, never a parser scope. */
char *csoundModuleSignature(CSOUND *csound, const char *signature, CS_SIGNATURE_MODE mode)
{
    if (signature == NULL) return NULL;
    char *result = csound->Malloc(csound, strlen(signature) + 1);
    size_t used = 0;
    for (const char *p = signature; *p;) {
        const char *end = *p == ':' ? strchr(p, ';') : NULL;
        size_t length = end != NULL ? (size_t)(end - p) + 1 : 1;
        char *token = cs_strndup(csound, p, length);
        const char *replacement = token;
        const CS_TYPE *type = NULL;
        if (end != NULL) {
            if (mode == CS_SIGNATURE_PUBLIC) {
                MODULE_STATE *state = csound->module_state;
                if (state != NULL && state->type_names != NULL)
                    type = cs_hash_table_get(csound, state->type_names, token);
                if (type != NULL) replacement = type->varTypeName;
            } else {
                MODULE_STATE *state = csound->module_state;
                if (mode == CS_SIGNATURE_PLUGIN && state != NULL && state->current_module != NULL &&
                    csoundModuleOwner(state->current_module) == state->root_module)
                    type = csoundResolveModuleType(csound, token);
                else type = csoundGetTypeWithVarTypeName(csound->typePool, token);
                if (type != NULL && csoundModuleTypeSignature(csound, type) != type->varTypeName)
                    replacement = csoundModuleTypeSignature(csound, type);
            }
        }
        size_t size = strlen(replacement);
        result = csound->ReAlloc(csound, result, used + size + 1);
        memcpy(result + used, replacement, size);
        used += size;
        csound->Free(csound, token);
        p += length;
    }
    result[used] = '\0';
    return result;
}

const OENTRY *csoundPublicOpcode(CSOUND *csound, const OENTRY *entry)
{
    if (entry == NULL ||
        ((entry->intypes == NULL || strstr(entry->intypes, ":#module_") == NULL) &&
         (entry->outypes == NULL || strstr(entry->outypes, ":#module_") == NULL)))
        return entry;
    MODULE_STATE *state = csoundGetModuleState(csound);
    if (state->public_opcodes == NULL) state->public_opcodes = cs_hash_table_create(csound);
    char key[2 * sizeof(void *) + 3];
    pointer_key(key, sizeof(key), entry);
    OENTRY *view = cs_hash_table_get(csound, state->public_opcodes, key);
    char *inputs, *outputs;
    if (view == NULL) {
        view = csound->Calloc(csound, sizeof(*view));
        inputs = csoundModuleSignature(csound, entry->intypes, CS_SIGNATURE_PUBLIC);
        outputs = csoundModuleSignature(csound, entry->outypes, CS_SIGNATURE_PUBLIC);
        cs_hash_table_put(csound, state->public_opcodes, key, view);
    } else { inputs = view->intypes; outputs = view->outypes; }
    *view = *entry;
    view->intypes = inputs;
    view->outypes = outputs;
    return view;
}

/* Plugin declarations keep public type names; compiler candidates use the
   root type identity current at the call site. Never mutate a plugin entry. */
OENTRY *csoundCompilerOpcode(CSOUND *csound, OENTRY *entry)
{
    MODULE_STATE *state = csound->module_state;
    if (state == NULL || state->type_names == NULL || entry->useropinfo != NULL ||
        (entry->intypes != NULL && strstr(entry->intypes, ":#module_") != NULL) ||
        (entry->outypes != NULL && strstr(entry->outypes, ":#module_") != NULL) ||
        ((entry->intypes == NULL || strchr(entry->intypes, ':') == NULL) &&
         (entry->outypes == NULL || strchr(entry->outypes, ':') == NULL))) return entry;
    char *inputs = csoundModuleSignature(csound, entry->intypes, CS_SIGNATURE_PLUGIN);
    char *outputs = csoundModuleSignature(csound, entry->outypes, CS_SIGNATURE_PLUGIN);
    if ((inputs == NULL || !strcmp(inputs, entry->intypes)) &&
        (outputs == NULL || !strcmp(outputs, entry->outypes))) {
        csound->Free(csound, inputs);
        csound->Free(csound, outputs);
        return entry;
    }
    size_t size = strlen(inputs != NULL ? inputs : "") +
                  strlen(outputs != NULL ? outputs : "") + 2 * sizeof(void *) + 6;
    char *key = csound->Malloc(csound, size);
    snprintf(key, size, "%p|%s|%s", (void *)entry,
             inputs != NULL ? inputs : "", outputs != NULL ? outputs : "");
    if (state->compiler_opcodes == NULL) state->compiler_opcodes = cs_hash_table_create(csound);
    OENTRY *candidate = cs_hash_table_get(csound, state->compiler_opcodes, key);
    if (candidate == NULL) {
        candidate = csound->Malloc(csound, sizeof(*candidate));
        *candidate = *entry;
        candidate->intypes = inputs;
        candidate->outypes = outputs;
        cs_hash_table_put(csound, state->compiler_opcodes, key, candidate);
    } else {
        csound->Free(csound, inputs);
        csound->Free(csound, outputs);
        inputs = candidate->intypes;
        outputs = candidate->outypes;
        *candidate = *entry;
        candidate->intypes = inputs;
        candidate->outypes = outputs;
    }
    csound->Free(csound, key);
    return candidate;
}
