/*
  wasm_plugins.c:

  Copyright (C) 2026 The Csound Developers

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound loads portable opcode WebAssembly here. Wasmtime compiles raw Wasm
  and owns its private, target-specific compilation cache.
*/

#include "csoundCore.h"
#include "Top/wasm_opcode_abi.h"
#include "wasm_plugins.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wasmtime.h>

#ifndef WASMTIME_FEATURE_COMPILER
#error "Native Csound Wasm plugins require a compiler-enabled Wasmtime C API"
#endif

#ifndef WASMTIME_FEATURE_GC
#error "Native Csound Wasm plugins require Wasmtime C API funcref support"
#endif

#ifndef WASMTIME_FEATURE_CACHE
#error "Native Csound Wasm plugins require Wasmtime compilation cache support"
#endif

enum {
  WASM_PAGE_SIZE = 65536,
  WASM_HOST_CSOUND_ADDRESS = 0x1000,
  WASM_HOST_OPCODE_LIST_SLOT = 0x2000,
  WASM_HOST_HEAP_START = 0x10000,
  WASM_HOST_HEAP_LIMIT = 64 * 1024 * 1024,
  WASM_PLUGIN_GLOBAL_BASE = 128 * 1024 * 1024,
  WASM_HOST_TABLE_ENTRIES = 3837,
  WASM_PLUGIN_TABLE_BASE = 4096,
  WASM_MAX_MEMORY_BYTES = 256 * 1024 * 1024,
  WASM_MAX_TABLE_ELEMENTS = 65536,
  WASM_MAX_MODULE_BYTES = 64 * 1024 * 1024,
  WASM_MAX_OPCODES = 256,
  WASM_MAX_ARGS = 64,
  WASM_MAX_NAME = 255,
  WASM_MAX_TYPE_STRING = 127
};

/* Store Wasmtime state per instance without adding a field to CSOUND_. */
#define WASM_REGISTRY_NAME "::csound_wasmtime_opcode_registry"

typedef struct WasmModule_s WasmModule;
typedef struct WasmOpcodeDescriptor_s WasmOpcodeDescriptor;
typedef struct WasmFreeBlock_s WasmFreeBlock;

typedef struct {
  wasmtime_memory_t memory;
  uint32_t next_address;
  size_t memory_size;
} WasmHostState;

typedef struct {
  wasmtime_store_t *store;
  wasmtime_context_t *context;
  wasmtime_instance_t instance;
  wasmtime_memory_t memory;
  wasmtime_table_t table;
  WasmHostState host;
} WasmGuest;

struct WasmFreeBlock_s {
  WasmFreeBlock *next;
  uint32_t address;
  uint32_t size;
};

struct WasmOpcodeDescriptor_s {
  WasmModule *module;
  uint32_t entry_index;
  uint32_t guest_dsblksiz;
  int32_t flags;
  int32_t deprecated;
  uint32_t init_index;
  uint32_t perf_index;
  uint32_t deinit_index;
  uint32_t guest_entry;
  uint32_t output_count;
  uint32_t argument_count;
  char *name;
  char *outypes;
  char *intypes;
  char argument_types[WASM_MAX_ARGS];
};

struct WasmModule_s {
  WasmModule *next;
  char *path;
  wasmtime_module_t *code;
  WasmGuest guest;
  WasmOpcodeDescriptor *opcodes;
  WasmFreeBlock *free_blocks;
  void *mutex;
  uint32_t opcode_count;
  uint32_t loader_memory_bytes;
  uint32_t loader_table_entries;
  uint32_t guest_argument_types[3];
  int32_t registered;
};

typedef struct {
  uint32_t memory_bytes;
  uint32_t table_entries;
  int32_t have_marker;
  int32_t have_layout;
} WasmPluginMetadata;

typedef struct {
  wasm_engine_t *engine;
  WasmModule *modules;
} WasmRegistry;

typedef struct {
  uint32_t address;
  uint32_t capacity;
} WasmArgumentBuffer;

typedef struct {
  WasmModule *module;
  wasmtime_func_t init;
  wasmtime_func_t perf;
  wasmtime_func_t deinit;
  uint32_t guest_opcode;
  uint32_t guest_insds;
  WasmArgumentBuffer arguments[WASM_MAX_ARGS];
  WasmFreeBlock *allocation;
  int32_t has_init;
  int32_t has_perf;
  int32_t has_deinit;
  int32_t guest_initialized;
  int32_t failed;
} WasmVoice;

/* Csound writes opcode argument pointers directly after OPDS. Keep the host
   state after a fixed upper bound so every supported signature has one native
   trampoline layout. */
typedef struct {
  OPDS h;
  void *arguments[WASM_MAX_ARGS];
  WasmOpcodeDescriptor *descriptor;
  WasmVoice *voice;
} WasmOpcodeInstance;

static uint32_t load_u32_le(const uint8_t *data, uint32_t offset)
{
  const uint8_t *p = data + offset;
  return ((uint32_t) p[0]) | ((uint32_t) p[1] << 8) |
         ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

static int32_t load_i32_le(const uint8_t *data, uint32_t offset)
{
  return (int32_t) load_u32_le(data, offset);
}

static void store_u32_le(uint8_t *data, uint32_t offset, uint32_t value)
{
  uint8_t *p = data + offset;
  p[0] = (uint8_t) value;
  p[1] = (uint8_t) (value >> 8);
  p[2] = (uint8_t) (value >> 16);
  p[3] = (uint8_t) (value >> 24);
}

static void store_f64_le(uint8_t *data, uint32_t offset, double value)
{
  union {
    double value;
    uint64_t bits;
  } item;
  uint32_t i;
  item.value = value;
  for (i = 0; i < 8; i++)
    data[offset + i] = (uint8_t) (item.bits >> (i * 8));
}

static double load_f64_le(const uint8_t *data, uint32_t offset)
{
  union {
    double value;
    uint64_t bits;
  } item;
  uint32_t i;
  item.bits = 0;
  for (i = 0; i < 8; i++)
    item.bits |= ((uint64_t) data[offset + i]) << (i * 8);
  return item.value;
}

static int32_t range_is_valid(size_t size, uint32_t offset, size_t length)
{
  return ((size_t) offset <= size && length <= size - (size_t) offset);
}

static int32_t wasm_name_equals(const wasm_name_t *name, const char *text)
{
  size_t length = strlen(text);
  return name != NULL && name->size == length &&
         memcmp(name->data, text, length) == 0;
}

static int32_t read_uleb32(const uint8_t *bytes, size_t size, size_t *position,
                           uint32_t *value)
{
  uint32_t result = 0;
  uint32_t shift = 0;
  while (*position < size && shift < 35) {
    uint8_t byte = bytes[(*position)++];
    result |= ((uint32_t) (byte & 0x7f)) << shift;
    if ((byte & 0x80) == 0) {
      *value = result;
      return 1;
    }
    shift += 7;
  }
  return 0;
}

static int32_t read_opcode_wasm_metadata(const uint8_t *bytes, size_t size,
                                         WasmPluginMetadata *metadata)
{
  static const uint8_t magic[8] = {0x00, 0x61, 0x73, 0x6d,
                                   0x01, 0x00, 0x00, 0x00};
  static const char marker[] = "OPCODE.WASM";
  static const char value[] = "Built by OPCODE.WASM";
  static const char layout[] = "dylink";
  size_t position = sizeof(magic);

  memset(metadata, 0, sizeof(*metadata));
  if (size < sizeof(magic) || memcmp(bytes, magic, sizeof(magic)) != 0)
    return 0;

  while (position < size) {
    uint8_t section_id = bytes[position++];
    uint32_t section_size;
    size_t section_end;
    if (!read_uleb32(bytes, size, &position, &section_size) ||
        section_size > size - position)
      return 0;
    section_end = position + section_size;
    if (section_id == 0) {
      uint32_t name_size;
      size_t payload;
      if (!read_uleb32(bytes, section_end, &position, &name_size) ||
          name_size > section_end - position)
        return 0;
      payload = position + name_size;
      if (name_size == sizeof(marker) - 1 &&
          memcmp(bytes + position, marker, sizeof(marker) - 1) == 0 &&
          section_end - payload == sizeof(value) - 1 &&
          memcmp(bytes + payload, value, sizeof(value) - 1) == 0) {
        if (metadata->have_marker)
          return 0;
        metadata->have_marker = 1;
      }
      else if (name_size == sizeof(layout) - 1 &&
               memcmp(bytes + position, layout, sizeof(layout) - 1) == 0) {
        uint32_t memory_alignment, table_alignment, needed_libraries;
        if (metadata->have_layout ||
            !read_uleb32(bytes, section_end, &payload,
                         &metadata->memory_bytes) ||
            !read_uleb32(bytes, section_end, &payload, &memory_alignment) ||
            !read_uleb32(bytes, section_end, &payload,
                         &metadata->table_entries) ||
            !read_uleb32(bytes, section_end, &payload, &table_alignment) ||
            !read_uleb32(bytes, section_end, &payload, &needed_libraries) ||
            payload != section_end || memory_alignment != 0 ||
            table_alignment != 0 || needed_libraries != 0)
          return 0;
        metadata->have_layout = 1;
      }
    }
    position = section_end;
  }
  return metadata->have_marker && metadata->have_layout;
}

static void print_wasmtime_error(CSOUND *csound, const char *action,
                                 wasmtime_error_t *error, wasm_trap_t *trap)
{
  wasm_name_t message;
  if (error != NULL) {
    wasmtime_error_message(error, &message);
    csound->ErrorMsg(csound, "%s: %.*s\n", action, (int) message.size,
                     message.data);
    wasm_byte_vec_delete(&message);
    wasmtime_error_delete(error);
  }
  if (trap != NULL) {
    wasm_trap_message(trap, &message);
    csound->ErrorMsg(csound, "%s trapped: %.*s\n", action,
                     (int) message.size, message.data);
    wasm_byte_vec_delete(&message);
    wasm_trap_delete(trap);
  }
}

static void discard_wasmtime_error(wasmtime_error_t *error, wasm_trap_t *trap)
{
  if (error != NULL)
    wasmtime_error_delete(error);
  if (trap != NULL)
    wasm_trap_delete(trap);
}

static uint8_t *read_module_file(const char *path, size_t *size)
{
  FILE *file = fopen(path, "rb");
  uint8_t *bytes = NULL;
  long end;

  if (file == NULL)
    return NULL;
  if (fseek(file, 0, SEEK_END) != 0 || (end = ftell(file)) <= 0 ||
      (uint64_t) end > WASM_MAX_MODULE_BYTES ||
      fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }
  bytes = (uint8_t *) malloc((size_t) end);
  if (bytes == NULL || fread(bytes, 1, (size_t) end, file) != (size_t) end) {
    free(bytes);
    fclose(file);
    return NULL;
  }
  fclose(file);
  *size = (size_t) end;
  return bytes;
}

static WasmRegistry *query_registry(CSOUND *csound)
{
  WasmRegistry **slot = (WasmRegistry **) csound->QueryGlobalVariable(
      csound, WASM_REGISTRY_NAME);
  return slot == NULL ? NULL : *slot;
}

static WasmRegistry *create_registry(CSOUND *csound)
{
  WasmRegistry **slot;
  WasmRegistry *registry = NULL;
  wasm_config_t *config;

  registry = query_registry(csound);
  if (registry != NULL)
    return registry;
  if (csound->CreateGlobalVariable(csound, WASM_REGISTRY_NAME,
                                   sizeof(WasmRegistry *)) != 0)
    return NULL;
  slot = (WasmRegistry **) csound->QueryGlobalVariable(csound,
                                                        WASM_REGISTRY_NAME);
  if (slot == NULL)
    goto fail;
  registry = (WasmRegistry *) csound->Calloc(csound, sizeof(*registry));
  if (registry == NULL)
    goto fail;

  config = wasm_config_new();
  if (config == NULL)
    goto fail;
  wasmtime_config_memory_reservation_set(config, WASM_MAX_MEMORY_BYTES);
  wasmtime_config_memory_may_move_set(config, false);
  wasmtime_config_memory_guard_size_set(config, WASM_PAGE_SIZE);
  wasmtime_config_memory_reservation_for_growth_set(config, 0);
  {
    const char *cache_config =
        csound->GetEnv(csound, "CSOUND_WASMTIME_CACHE_CONFIG");
    wasmtime_error_t *error = wasmtime_config_cache_config_load(
        config, cache_config != NULL && cache_config[0] != '\0'
                    ? cache_config
                    : NULL);
    if (error != NULL) {
      wasm_name_t message;
      wasmtime_error_message(error, &message);
      csound->Warning(csound,
                      "Wasmtime compilation cache is unavailable: %.*s\n",
                      (int) message.size, message.data);
      wasm_byte_vec_delete(&message);
      wasmtime_error_delete(error);
    }
  }
  registry->engine = wasm_engine_new_with_config(config);
  if (registry->engine == NULL)
    goto fail;
  *slot = registry;
  return registry;

fail:
  if (registry != NULL)
    csound->Free(csound, registry);
  csound->DestroyGlobalVariable(csound, WASM_REGISTRY_NAME);
  return NULL;
}

static wasm_trap_t *host_calloc(void *environment, wasmtime_caller_t *caller,
                                const wasmtime_val_t *args, size_t nargs,
                                wasmtime_val_t *results, size_t nresults)
{
  WasmHostState *state = (WasmHostState *) environment;
  wasmtime_context_t *context = wasmtime_caller_context(caller);
  uint8_t *memory = wasmtime_memory_data(context, &state->memory);
  size_t memory_size = wasmtime_memory_data_size(context, &state->memory);
  uint32_t address;
  uint32_t allocation_size;

  if (nargs != 2 || nresults != 1 || args[1].kind != WASMTIME_I32)
    return NULL;
  allocation_size = (uint32_t) args[1].of.i32;
  address = (state->next_address + 7u) & ~7u;
  results[0].kind = WASMTIME_I32;
  results[0].of.i32 = 0;
  if (address < state->next_address ||
      (uint64_t) address + allocation_size > memory_size ||
      (uint64_t) address + allocation_size > WASM_HOST_HEAP_LIMIT)
    return NULL;
  memset(memory + address, 0, allocation_size);
  state->next_address = address + allocation_size;
  state->memory_size = memory_size;
  results[0].of.i32 = (int32_t) address;
  return NULL;
}

static int32_t call_function(CSOUND *csound, wasmtime_context_t *context,
                             const wasmtime_func_t *function,
                             const wasmtime_val_t *args, size_t nargs,
                             wasmtime_val_t *results, size_t nresults,
                             const char *action, int32_t report)
{
  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error = wasmtime_func_call(
      context, function, args, nargs, results, nresults, &trap);
  if (error == NULL && trap == NULL)
    return CSOUND_SUCCESS;
  if (report)
    print_wasmtime_error(csound, action, error, trap);
  else
    discard_wasmtime_error(error, trap);
  return CSOUND_ERROR;
}

static int32_t get_exported_function(wasmtime_context_t *context,
                                     const wasmtime_instance_t *instance,
                                     const char *name,
                                     wasmtime_func_t *function)
{
  wasmtime_extern_t item;
  if (!wasmtime_instance_export_get(context, instance, name, strlen(name),
                                    &item))
    return CSOUND_ERROR;
  if (item.kind != WASMTIME_EXTERN_FUNC) {
    wasmtime_extern_delete(&item);
    return CSOUND_ERROR;
  }
  *function = item.of.func;
  wasmtime_extern_delete(&item);
  return CSOUND_SUCCESS;
}

static int32_t get_table_function(wasmtime_context_t *context,
                                  const wasmtime_table_t *table,
                                  uint32_t index, wasmtime_func_t *function)
{
  wasmtime_val_t value;
  wasm_functype_t *type;
  const wasm_valtype_vec_t *parameters;
  const wasm_valtype_vec_t *results;
  if (!wasmtime_table_get(context, table, index, &value))
    return CSOUND_ERROR;
  if (value.kind != WASMTIME_FUNCREF ||
      wasmtime_funcref_is_null(&value.of.funcref)) {
    wasmtime_val_unroot(&value);
    return CSOUND_ERROR;
  }
  *function = value.of.funcref;
  wasmtime_val_unroot(&value);
  type = wasmtime_func_type(context, function);
  if (type == NULL)
    return CSOUND_ERROR;
  parameters = wasm_functype_params(type);
  results = wasm_functype_results(type);
  if (parameters->size != 2 || results->size != 1 ||
      wasm_valtype_kind(parameters->data[0]) != WASM_I32 ||
      wasm_valtype_kind(parameters->data[1]) != WASM_I32 ||
      wasm_valtype_kind(results->data[0]) != WASM_I32) {
    wasm_functype_delete(type);
    return CSOUND_ERROR;
  }
  wasm_functype_delete(type);
  return CSOUND_SUCCESS;
}

static void destroy_guest(WasmGuest *guest)
{
  if (guest->store != NULL)
    wasmtime_store_delete(guest->store);
  memset(guest, 0, sizeof(*guest));
}

static int32_t create_guest(CSOUND *csound, WasmModule *module,
                            WasmGuest *guest)
{
  wasm_importtype_vec_t import_types;
  wasmtime_extern_t *imports = NULL;
  size_t i;
  int32_t have_memory = 0;
  int32_t have_table = 0;
  int32_t result = CSOUND_ERROR;

  memset(guest, 0, sizeof(*guest));
  guest->store = wasmtime_store_new(query_registry(csound)->engine, NULL, NULL);
  if (guest->store == NULL)
    return CSOUND_ERROR;
  guest->context = wasmtime_store_context(guest->store);
  wasmtime_store_limiter(guest->store, WASM_MAX_MEMORY_BYTES,
                         WASM_MAX_TABLE_ELEMENTS, 1, 1, 1);

  wasmtime_module_imports(module->code, &import_types);
  if (import_types.size != 2) {
    csound->ErrorMsg(csound,
                     "Wasm opcode library '%s' must import only env.memory "
                     "and env.__indirect_function_table\n",
                     module->path);
    goto done;
  }
  imports = (wasmtime_extern_t *) calloc(import_types.size, sizeof(*imports));
  if (imports == NULL)
    goto done;

  for (i = 0; i < import_types.size; i++) {
    const wasm_importtype_t *import_type = import_types.data[i];
    const wasm_name_t *import_module = wasm_importtype_module(import_type);
    const wasm_name_t *import_name = wasm_importtype_name(import_type);
    const wasm_externtype_t *external_type =
        wasm_importtype_type(import_type);
    wasm_externkind_t kind = wasm_externtype_kind(external_type);

    if (!wasm_name_equals(import_module, "env"))
      goto unsupported_import;
    if (kind == WASM_EXTERN_MEMORY &&
        wasm_name_equals(import_name, "memory") && !have_memory) {
      const wasm_memorytype_t *memory_type =
          wasm_externtype_as_memorytype_const(external_type);
      uint64_t minimum = wasmtime_memorytype_minimum(memory_type);
      uint64_t expected_bytes =
          (uint64_t) WASM_PLUGIN_GLOBAL_BASE + module->loader_memory_bytes;
      wasmtime_error_t *error;
      if (wasmtime_memorytype_is64(memory_type) ||
          wasmtime_memorytype_isshared(memory_type) ||
          expected_bytes % WASM_PAGE_SIZE != 0 ||
          minimum != expected_bytes / WASM_PAGE_SIZE ||
          minimum > WASM_MAX_MEMORY_BYTES / WASM_PAGE_SIZE) {
        csound->ErrorMsg(csound,
                         "Wasm opcode library '%s' has an unsupported "
                         "memory layout\n",
                         module->path);
        goto done;
      }
      error = wasmtime_memory_new(guest->context, memory_type, &guest->memory);
      if (error != NULL) {
        print_wasmtime_error(csound, "create Wasm plugin memory", error, NULL);
        goto done;
      }
      imports[i].kind = WASMTIME_EXTERN_MEMORY;
      imports[i].of.memory = guest->memory;
      have_memory = 1;
    }
    else if (kind == WASM_EXTERN_TABLE &&
             wasm_name_equals(import_name, "__indirect_function_table") &&
             !have_table) {
      const wasm_tabletype_t *table_type =
          wasm_externtype_as_tabletype_const(external_type);
      const wasm_limits_t *limits = wasm_tabletype_limits(table_type);
      const wasm_valtype_t *element = wasm_tabletype_element(table_type);
      uint64_t expected_minimum =
          (uint64_t) WASM_HOST_TABLE_ENTRIES +
          module->loader_table_entries;
      wasmtime_val_t null_function = {.kind = WASMTIME_FUNCREF};
      wasmtime_error_t *error;
      if (wasm_valtype_kind(element) != WASM_FUNCREF ||
          expected_minimum <= WASM_PLUGIN_TABLE_BASE ||
          limits->min != expected_minimum ||
          limits->min > WASM_MAX_TABLE_ELEMENTS) {
        csound->ErrorMsg(csound,
                         "Wasm opcode library '%s' has an unsupported "
                         "function table layout\n",
                         module->path);
        goto done;
      }
      wasmtime_funcref_set_null(&null_function.of.funcref);
      error = wasmtime_table_new(guest->context, table_type, &null_function,
                                 &guest->table);
      if (error != NULL) {
        print_wasmtime_error(csound, "create Wasm plugin table", error, NULL);
        goto done;
      }
      imports[i].kind = WASMTIME_EXTERN_TABLE;
      imports[i].of.table = guest->table;
      have_table = 1;
    }
    else {
unsupported_import:
      csound->ErrorMsg(csound, "Unsupported Wasm import in '%s': %.*s.%.*s\n",
                       module->path, (int) import_module->size,
                       import_module->data, (int) import_name->size,
                       import_name->data);
      goto done;
    }
  }

  if (!have_memory || !have_table)
    goto done;
  {
    wasm_trap_t *trap = NULL;
    wasmtime_error_t *error = wasmtime_instance_new(
        guest->context, module->code, imports, import_types.size,
        &guest->instance, &trap);
    if (error != NULL || trap != NULL) {
      print_wasmtime_error(csound, "instantiate Wasm opcode library", error,
                           trap);
      goto done;
    }
  }

  guest->host.memory = guest->memory;
  guest->host.next_address = WASM_HOST_HEAP_START;
  guest->host.memory_size =
      wasmtime_memory_data_size(guest->context, &guest->memory);
  if (!range_is_valid(guest->host.memory_size, WASM_HOST_CSOUND_ADDRESS, 1024) ||
      !range_is_valid(guest->host.memory_size, WASM_HOST_OPCODE_LIST_SLOT, 4)) {
    csound->ErrorMsg(csound, "Wasm memory in '%s' is too small\n", module->path);
    goto done;
  }
  {
    wasm_functype_t *type = wasm_functype_new_2_1(
        wasm_valtype_new_i32(), wasm_valtype_new_i32(),
        wasm_valtype_new_i32());
    wasmtime_func_t function;
    wasmtime_val_t value = {.kind = WASMTIME_FUNCREF};
    wasmtime_error_t *error;
    if (type == NULL)
      goto done;
    wasmtime_func_new(guest->context, type, host_calloc, &guest->host, NULL,
                      &function);
    wasm_functype_delete(type);
    value.of.funcref = function;
    error = wasmtime_table_set(guest->context, &guest->table, 0, &value);
    if (error != NULL) {
      print_wasmtime_error(csound, "install Wasm plugin Calloc", error, NULL);
      goto done;
    }
  }
  {
    uint8_t *memory = wasmtime_memory_data(guest->context, &guest->memory);
    memset(memory + WASM_HOST_CSOUND_ADDRESS, 0, 1024);
    store_u32_le(memory,
                 WASM_HOST_CSOUND_ADDRESS + WASM32_CSOUND_CALLOC_OFFSET, 0);
    store_u32_le(memory, WASM_HOST_OPCODE_LIST_SLOT, 0);
  }
  result = CSOUND_SUCCESS;

done:
  free(imports);
  wasm_importtype_vec_delete(&import_types);
  if (result != CSOUND_SUCCESS)
    destroy_guest(guest);
  return result;
}

static int32_t module_info_is_compatible(int32_t info)
{
  int32_t float_size = info & 0xff;
  int32_t minor = (info >> 8) & 0xff;
  int32_t major = (info >> 16) & 0xffff;
  return float_size == (int32_t) sizeof(MYFLT) && major == CS_VERSION &&
         minor <= CS_SUBVER;
}

static int32_t prepare_guest(CSOUND *csound, WasmModule *module,
                             WasmGuest *guest, uint32_t *opcode_list,
                             uint32_t *opcode_count)
{
  wasmtime_func_t function;
  wasmtime_val_t result;
  uint8_t *memory;
  size_t memory_size;

  if (create_guest(csound, module, guest) != CSOUND_SUCCESS)
    return CSOUND_ERROR;

  if (get_exported_function(guest->context, &guest->instance,
                            "__wasm_call_ctors", &function) == CSOUND_SUCCESS &&
      call_function(csound, guest->context, &function, NULL, 0, NULL, 0,
                    "run Wasm opcode constructors", 1) != CSOUND_SUCCESS)
    goto fail;

  if (get_exported_function(guest->context, &guest->instance,
                            "csoundModuleInfo", &function) != CSOUND_SUCCESS) {
    csound->ErrorMsg(csound, "Wasm opcode library '%s' has no csoundModuleInfo\n",
                     module->path);
    goto fail;
  }
  result.kind = WASMTIME_I32;
  if (call_function(csound, guest->context, &function, NULL, 0, &result, 1,
                    "read Wasm opcode module info", 1) != CSOUND_SUCCESS ||
      !module_info_is_compatible(result.of.i32)) {
    csound->ErrorMsg(csound,
                     "Wasm opcode library '%s' is not compatible with this "
                     "Csound build\n",
                     module->path);
    goto fail;
  }

  if (get_exported_function(guest->context, &guest->instance,
                            "csound_opcode_init", &function) != CSOUND_SUCCESS) {
    csound->ErrorMsg(csound,
                     "Wasm opcode library '%s' has no csound_opcode_init\n",
                     module->path);
    goto fail;
  }
  {
    wasmtime_val_t arguments[2] = {
        {.kind = WASMTIME_I32, .of.i32 = WASM_HOST_CSOUND_ADDRESS},
        {.kind = WASMTIME_I32, .of.i32 = WASM_HOST_OPCODE_LIST_SLOT}};
    result.kind = WASMTIME_I64;
    if (call_function(csound, guest->context, &function, arguments, 2, &result,
                      1, "read Wasm opcode list", 1) != CSOUND_SUCCESS ||
        result.of.i64 <= 0 || result.of.i64 % WASM32_OENTRY_SIZE != 0 ||
        result.of.i64 / WASM32_OENTRY_SIZE > WASM_MAX_OPCODES) {
      csound->ErrorMsg(csound, "Invalid opcode list in '%s'\n", module->path);
      goto fail;
    }
    *opcode_count = (uint32_t) (result.of.i64 / WASM32_OENTRY_SIZE);
  }

  memory = wasmtime_memory_data(guest->context, &guest->memory);
  memory_size = wasmtime_memory_data_size(guest->context, &guest->memory);
  *opcode_list = load_u32_le(memory, WASM_HOST_OPCODE_LIST_SLOT);
  if (!range_is_valid(memory_size, *opcode_list,
                      (size_t) *opcode_count * WASM32_OENTRY_SIZE)) {
    csound->ErrorMsg(csound, "Invalid opcode list address in '%s'\n",
                     module->path);
    goto fail;
  }
  return CSOUND_SUCCESS;

fail:
  destroy_guest(guest);
  return CSOUND_ERROR;
}

static char *copy_guest_string(CSOUND *csound, const uint8_t *memory,
                               size_t memory_size, uint32_t address,
                               size_t maximum)
{
  size_t length = 0;
  char *copy;
  if (address == 0 || (size_t) address >= memory_size)
    return NULL;
  while (length <= maximum && (size_t) address + length < memory_size &&
         memory[address + length] != '\0')
    length++;
  if (length > maximum || (size_t) address + length >= memory_size)
    return NULL;
  copy = (char *) csound->Malloc(csound, length + 1);
  if (copy == NULL)
    return NULL;
  memcpy(copy, memory + address, length);
  copy[length] = '\0';
  return copy;
}

static int32_t guest_string_equals(const uint8_t *memory, size_t memory_size,
                                   uint32_t address, const char *text,
                                   size_t maximum)
{
  size_t length = strlen(text);
  return address != 0 && length <= maximum &&
         range_is_valid(memory_size, address, length + 1) &&
         memcmp(memory + address, text, length) == 0 &&
         memory[address + length] == '\0';
}

static int32_t parse_type_string(const char *types, char *arguments,
                                 uint32_t *count)
{
  const char *p;
  for (p = types; *p != '\0'; p++) {
    if (*p != 'a' && *p != 'k' && *p != 'i')
      return CSOUND_ERROR;
    if (*count >= WASM_MAX_ARGS)
      return CSOUND_ERROR;
    arguments[(*count)++] = *p;
  }
  return CSOUND_SUCCESS;
}

static uint32_t guest_allocate(WasmGuest *guest, uint32_t size,
                               uint32_t alignment);

static int32_t prepare_argument_types(WasmModule *module)
{
  static const char types[3] = {'a', 'k', 'i'};
  WasmGuest *guest = &module->guest;
  uint8_t *memory = wasmtime_memory_data(guest->context, &guest->memory);
  uint32_t i;
  for (i = 0; i < 3; i++) {
    uint32_t record = guest_allocate(guest, WASM32_CS_TYPE_SIZE + 2, 8);
    uint32_t name;
    if (record == 0)
      return CSOUND_MEMORY;
    name = record + WASM32_CS_TYPE_SIZE;
    store_u32_le(memory, record, name);
    memory[name] = (uint8_t) types[i];
    memory[name + 1] = '\0';
    module->guest_argument_types[i] = record;
  }
  return CSOUND_SUCCESS;
}

static uint32_t argument_type_record(const WasmModule *module, char type)
{
  if (type == 'a')
    return module->guest_argument_types[0];
  if (type == 'k')
    return module->guest_argument_types[1];
  return module->guest_argument_types[2];
}

static void free_descriptor(CSOUND *csound, WasmOpcodeDescriptor *descriptor)
{
  if (descriptor->name != NULL)
    csound->Free(csound, descriptor->name);
  if (descriptor->outypes != NULL)
    csound->Free(csound, descriptor->outypes);
  if (descriptor->intypes != NULL)
    csound->Free(csound, descriptor->intypes);
}

static int32_t discover_opcodes(CSOUND *csound, WasmModule *module)
{
  WasmGuest *guest = &module->guest;
  uint32_t opcode_list;
  uint32_t opcode_count;
  uint8_t *memory;
  size_t memory_size;
  uint32_t i;

  if (prepare_guest(csound, module, guest, &opcode_list, &opcode_count) !=
      CSOUND_SUCCESS)
    return CSOUND_ERROR;
  if (prepare_argument_types(module) != CSOUND_SUCCESS) {
    destroy_guest(guest);
    return CSOUND_MEMORY;
  }
  module->opcodes = (WasmOpcodeDescriptor *) csound->Calloc(
      csound, sizeof(*module->opcodes) * opcode_count);
  if (module->opcodes == NULL) {
    destroy_guest(guest);
    return CSOUND_MEMORY;
  }
  module->opcode_count = opcode_count;
  memory = wasmtime_memory_data(guest->context, &guest->memory);
  memory_size = wasmtime_memory_data_size(guest->context, &guest->memory);

  for (i = 0; i < opcode_count; i++) {
    WasmOpcodeDescriptor *descriptor = &module->opcodes[i];
    uint32_t entry = opcode_list + i * WASM32_OENTRY_SIZE;
    descriptor->module = module;
    descriptor->entry_index = i;
    descriptor->guest_entry = entry;
    descriptor->guest_dsblksiz =
        load_u32_le(memory, entry + WASM32_OENTRY_DSBLOCKSIZE_OFFSET);
    descriptor->flags =
        load_i32_le(memory, entry + WASM32_OENTRY_FLAGS_OFFSET);
    descriptor->init_index =
        load_u32_le(memory, entry + WASM32_OENTRY_INIT_OFFSET);
    descriptor->perf_index =
        load_u32_le(memory, entry + WASM32_OENTRY_PERF_OFFSET);
    descriptor->deinit_index =
        load_u32_le(memory, entry + WASM32_OENTRY_DEINIT_OFFSET);
    descriptor->deprecated =
        load_i32_le(memory, entry + WASM32_OENTRY_DEPRECATED_OFFSET);
    if (load_u32_le(memory, entry + WASM32_OENTRY_USEROPINFO_OFFSET) != 0)
      goto invalid;
    descriptor->name = copy_guest_string(
        csound, memory, memory_size,
        load_u32_le(memory, entry + WASM32_OENTRY_NAME_OFFSET), WASM_MAX_NAME);
    descriptor->outypes = copy_guest_string(
        csound, memory, memory_size,
        load_u32_le(memory, entry + WASM32_OENTRY_OUTYPES_OFFSET),
        WASM_MAX_TYPE_STRING);
    descriptor->intypes = copy_guest_string(
        csound, memory, memory_size,
        load_u32_le(memory, entry + WASM32_OENTRY_INTYPES_OFFSET),
        WASM_MAX_TYPE_STRING);
    if (descriptor->name == NULL || descriptor->outypes == NULL ||
        descriptor->intypes == NULL)
      goto invalid;
    if (parse_type_string(descriptor->outypes, descriptor->argument_types,
                          &descriptor->argument_count) != CSOUND_SUCCESS)
      goto invalid_type;
    descriptor->output_count = descriptor->argument_count;
    if (parse_type_string(descriptor->intypes, descriptor->argument_types,
                          &descriptor->argument_count) != CSOUND_SUCCESS)
      goto invalid_type;
    if (descriptor->guest_dsblksiz <
        WASM32_OPDS_SIZE + descriptor->argument_count * sizeof(uint32_t))
      goto invalid;
    {
      wasmtime_func_t callback;
      if ((descriptor->init_index != 0 &&
           get_table_function(guest->context, &guest->table,
                              descriptor->init_index, &callback) !=
               CSOUND_SUCCESS) ||
          (descriptor->perf_index != 0 &&
           get_table_function(guest->context, &guest->table,
                              descriptor->perf_index, &callback) !=
               CSOUND_SUCCESS) ||
          (descriptor->deinit_index != 0 &&
           get_table_function(guest->context, &guest->table,
                              descriptor->deinit_index, &callback) !=
               CSOUND_SUCCESS))
        goto invalid_callback;
    }
    continue;

invalid_callback:
    csound->ErrorMsg(csound, "Wasm opcode '%s' has an invalid callback\n",
                     descriptor->name);
    goto invalid;
invalid_type:
    csound->ErrorMsg(csound,
                     "Wasm opcode '%s' uses unsupported types '%s', '%s'\n",
                     descriptor->name, descriptor->outypes,
                     descriptor->intypes);
invalid:
    csound->ErrorMsg(csound, "Invalid Wasm opcode entry %u in '%s'\n", i,
                     module->path);
    while (i > 0)
      free_descriptor(csound, &module->opcodes[--i]);
    free_descriptor(csound, descriptor);
    csound->Free(csound, module->opcodes);
    module->opcodes = NULL;
    module->opcode_count = 0;
    destroy_guest(guest);
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

static int32_t signatures_match(const WasmOpcodeDescriptor *left,
                                const WasmOpcodeDescriptor *right)
{
  return strcmp(left->name, right->name) == 0 &&
         strcmp(left->outypes, right->outypes) == 0 &&
         strcmp(left->intypes, right->intypes) == 0;
}

static int32_t validate_unique_signatures(CSOUND *csound,
                                          const WasmRegistry *registry,
                                          const WasmModule *module)
{
  uint32_t i;
  for (i = 0; i < module->opcode_count; i++) {
    const WasmOpcodeDescriptor *descriptor = &module->opcodes[i];
    const WasmModule *other_module;
    uint32_t j;
    for (j = 0; j < i; j++) {
      if (signatures_match(descriptor, &module->opcodes[j]))
        goto duplicate_in_module;
    }
    for (other_module = registry->modules; other_module != NULL;
         other_module = other_module->next) {
      for (j = 0; j < other_module->opcode_count; j++) {
        if (signatures_match(descriptor, &other_module->opcodes[j])) {
          csound->ErrorMsg(
              csound,
              "Wasm opcode '%s' with signature '%s', '%s' is already loaded "
              "from '%s'\n",
              descriptor->name, descriptor->outypes, descriptor->intypes,
              other_module->path);
          return CSOUND_ERROR;
        }
      }
    }
  }
  return CSOUND_SUCCESS;

duplicate_in_module:
  csound->ErrorMsg(csound,
                   "Wasm opcode library '%s' repeats opcode '%s' with "
                   "signature '%s', '%s'\n",
                   module->path, module->opcodes[i].name,
                   module->opcodes[i].outypes, module->opcodes[i].intypes);
  return CSOUND_ERROR;
}

static void destroy_module(CSOUND *csound, WasmModule *module)
{
  WasmFreeBlock *block;
  uint32_t i;
  if (module == NULL)
    return;
  destroy_guest(&module->guest);
  if (module->mutex != NULL)
    csound->DestroyMutex(module->mutex);
  block = module->free_blocks;
  while (block != NULL) {
    WasmFreeBlock *next = block->next;
    csound->Free(csound, block);
    block = next;
  }
  for (i = 0; i < module->opcode_count; i++)
    free_descriptor(csound, &module->opcodes[i]);
  if (module->opcodes != NULL)
    csound->Free(csound, module->opcodes);
  if (module->code != NULL)
    wasmtime_module_delete(module->code);
  if (module->path != NULL)
    csound->Free(csound, module->path);
  csound->Free(csound, module);
}

static uint32_t guest_allocate(WasmGuest *guest, uint32_t size,
                               uint32_t alignment)
{
  uint32_t mask = alignment - 1;
  uint32_t address = (guest->host.next_address + mask) & ~mask;
  size_t memory_size = wasmtime_memory_data_size(guest->context,
                                                 &guest->memory);
  if (address < guest->host.next_address ||
      (uint64_t) address + size > memory_size ||
      (uint64_t) address + size > WASM_HOST_HEAP_LIMIT)
    return 0;
  guest->host.next_address = address + size;
  memset(wasmtime_memory_data(guest->context, &guest->memory) + address, 0,
         size);
  return address;
}

static WasmFreeBlock *allocate_voice_block(CSOUND *csound, WasmModule *module,
                                           uint32_t size)
{
  WasmFreeBlock **link = &module->free_blocks;
  WasmFreeBlock *block;
  while (*link != NULL && (*link)->size < size)
    link = &(*link)->next;
  if (*link != NULL) {
    WasmFreeBlock *free_block = *link;
    if (free_block->size == size) {
      *link = free_block->next;
      free_block->next = NULL;
      block = free_block;
    }
    else {
      block = (WasmFreeBlock *) csound->Malloc(csound, sizeof(*block));
      if (block == NULL)
        return NULL;
      block->next = NULL;
      block->address = free_block->address;
      block->size = size;
      free_block->address += size;
      free_block->size -= size;
    }
  }
  else {
    uint32_t address;
    block = (WasmFreeBlock *) csound->Malloc(csound, sizeof(*block));
    if (block == NULL)
      return NULL;
    address = guest_allocate(&module->guest, size, 8);
    if (address == 0) {
      csound->Free(csound, block);
      return NULL;
    }
    block->next = NULL;
    block->address = address;
    block->size = size;
  }
  memset(wasmtime_memory_data(module->guest.context, &module->guest.memory) +
             block->address,
         0, block->size);
  return block;
}

static void release_voice_block(CSOUND *csound, WasmModule *module,
                                WasmFreeBlock *block)
{
  WasmFreeBlock **link;
  WasmFreeBlock *previous = NULL;
  if (block == NULL)
    return;
  link = &module->free_blocks;
  while (*link != NULL && (*link)->address < block->address) {
    previous = *link;
    link = &(*link)->next;
  }
  block->next = *link;
  *link = block;
  if (previous != NULL &&
      (uint64_t) previous->address + previous->size == block->address) {
    previous->size += block->size;
    previous->next = block->next;
    csound->Free(csound, block);
    block = previous;
  }
  if (block->next != NULL &&
      (uint64_t) block->address + block->size == block->next->address) {
    WasmFreeBlock *next = block->next;
    block->size += next->size;
    block->next = next->next;
    csound->Free(csound, next);
  }
}

static WasmOpcodeDescriptor *find_descriptor(CSOUND *csound,
                                             WasmOpcodeInstance *opcode)
{
  WasmRegistry *registry = query_registry(csound);
  OENTRY *entry = opcode->h.optext == NULL ? NULL : opcode->h.optext->t.oentry;
  WasmModule *module;
  uint32_t i;
  if (registry == NULL || entry == NULL)
    return NULL;
  for (module = registry->modules; module != NULL; module = module->next) {
    for (i = 0; i < module->opcode_count; i++) {
      WasmOpcodeDescriptor *descriptor = &module->opcodes[i];
      if (strcmp(entry->opname, descriptor->name) == 0 &&
          strcmp(entry->outypes, descriptor->outypes) == 0 &&
          strcmp(entry->intypes, descriptor->intypes) == 0)
        return descriptor;
    }
  }
  return NULL;
}

static int32_t call_opcode_function(CSOUND *csound, WasmVoice *voice,
                                    const wasmtime_func_t *function,
                                    const char *action, int32_t report,
                                    int32_t *guest_result)
{
  WasmGuest *guest = &voice->module->guest;
  wasmtime_val_t arguments[2] = {
      {.kind = WASMTIME_I32, .of.i32 = WASM_HOST_CSOUND_ADDRESS},
      {.kind = WASMTIME_I32, .of.i32 = (int32_t) voice->guest_opcode}};
  wasmtime_val_t result = {.kind = WASMTIME_I32};
  if (call_function(csound, guest->context, function, arguments, 2,
                    &result, 1, action, report) != CSOUND_SUCCESS)
    return CSOUND_ERROR;
  *guest_result = result.of.i32;
  return CSOUND_SUCCESS;
}

static void destroy_voice(CSOUND *csound, WasmVoice *voice)
{
  WasmModule *module;
  if (voice == NULL)
    return;
  module = voice->module;
  if (module != NULL && module->mutex != NULL)
    csound->LockMutex(module->mutex);
  if (voice->guest_initialized && voice->has_deinit) {
    int32_t guest_result;
    (void) call_opcode_function(csound, voice, &voice->deinit,
                                "deinitialise Wasm opcode", 0, &guest_result);
  }
  if (module != NULL) {
    release_voice_block(csound, module, voice->allocation);
    voice->allocation = NULL;
  }
  if (module != NULL && module->mutex != NULL)
    csound->UnlockMutex(module->mutex);
  csound->Free(csound, voice);
}

static int32_t descriptor_matches_guest(const WasmOpcodeDescriptor *descriptor)
{
  WasmGuest *guest = &descriptor->module->guest;
  const uint8_t *memory =
      wasmtime_memory_data(guest->context, &guest->memory);
  size_t memory_size = wasmtime_memory_data_size(guest->context, &guest->memory);
  uint32_t entry = descriptor->guest_entry;
  return range_is_valid(memory_size, entry, WASM32_OENTRY_SIZE) &&
         load_u32_le(memory, entry + WASM32_OENTRY_DSBLOCKSIZE_OFFSET) ==
             descriptor->guest_dsblksiz &&
         load_i32_le(memory, entry + WASM32_OENTRY_FLAGS_OFFSET) ==
             descriptor->flags &&
         load_u32_le(memory, entry + WASM32_OENTRY_INIT_OFFSET) ==
             descriptor->init_index &&
         load_u32_le(memory, entry + WASM32_OENTRY_PERF_OFFSET) ==
             descriptor->perf_index &&
         load_u32_le(memory, entry + WASM32_OENTRY_DEINIT_OFFSET) ==
             descriptor->deinit_index &&
         load_u32_le(memory, entry + WASM32_OENTRY_USEROPINFO_OFFSET) == 0 &&
         load_i32_le(memory, entry + WASM32_OENTRY_DEPRECATED_OFFSET) ==
             descriptor->deprecated &&
         guest_string_equals(
             memory, memory_size,
             load_u32_le(memory, entry + WASM32_OENTRY_NAME_OFFSET),
             descriptor->name, WASM_MAX_NAME) &&
         guest_string_equals(
             memory, memory_size,
             load_u32_le(memory, entry + WASM32_OENTRY_OUTYPES_OFFSET),
             descriptor->outypes, WASM_MAX_TYPE_STRING) &&
         guest_string_equals(
             memory, memory_size,
             load_u32_le(memory, entry + WASM32_OENTRY_INTYPES_OFFSET),
             descriptor->intypes, WASM_MAX_TYPE_STRING);
}

static uint32_t voice_allocation_size(const WasmOpcodeDescriptor *descriptor,
                                      uint32_t ksmps)
{
  uint64_t size = WASM32_INSDS_PREFIX_SIZE;
  uint32_t i;
  size = (size + 7u) & ~(uint64_t) 7u;
  size += descriptor->guest_dsblksiz;
  size = (size + 7u) & ~(uint64_t) 7u;
  for (i = 0; i < descriptor->argument_count; i++) {
    size += WASM32_VAR_TYPE_OFFSET;
    size += descriptor->argument_types[i] == 'a'
                ? (uint64_t) ksmps * sizeof(MYFLT)
                : sizeof(MYFLT);
    size = (size + 7u) & ~(uint64_t) 7u;
  }
  return size > UINT32_MAX ? 0 : (uint32_t) size;
}

static WasmVoice *create_voice(CSOUND *csound,
                               WasmOpcodeDescriptor *descriptor,
                               uint32_t ksmps)
{
  WasmVoice *voice = (WasmVoice *) csound->Calloc(csound, sizeof(*voice));
  WasmModule *module = descriptor->module;
  WasmGuest *guest = &module->guest;
  uint8_t *memory;
  uint32_t cursor;
  uint32_t allocation_size;
  uint32_t i;

  if (voice == NULL)
    return NULL;
  if (module->mutex == NULL) {
    csound->Free(csound, voice);
    return NULL;
  }
  voice->module = module;
  csound->LockMutex(module->mutex);
  if (!descriptor_matches_guest(descriptor))
    goto fail;

  if (descriptor->init_index != 0) {
    if (get_table_function(guest->context, &guest->table,
                           descriptor->init_index, &voice->init) !=
        CSOUND_SUCCESS)
      goto fail;
    voice->has_init = 1;
  }
  if (descriptor->perf_index != 0) {
    if (get_table_function(guest->context, &guest->table,
                           descriptor->perf_index, &voice->perf) !=
        CSOUND_SUCCESS)
      goto fail;
    voice->has_perf = 1;
  }
  if (descriptor->deinit_index != 0) {
    if (get_table_function(guest->context, &guest->table,
                           descriptor->deinit_index, &voice->deinit) !=
        CSOUND_SUCCESS)
      goto fail;
    voice->has_deinit = 1;
  }

  allocation_size = voice_allocation_size(descriptor, ksmps);
  if (allocation_size == 0 ||
      (voice->allocation =
           allocate_voice_block(csound, module, allocation_size)) == NULL)
    goto fail;
  cursor = voice->allocation->address;
  voice->guest_insds = cursor;
  cursor = (cursor + WASM32_INSDS_PREFIX_SIZE + 7u) & ~7u;
  voice->guest_opcode = cursor;
  cursor = (cursor + descriptor->guest_dsblksiz + 7u) & ~7u;
  memory = wasmtime_memory_data(guest->context, &guest->memory);
  store_u32_le(memory,
               voice->guest_opcode + WASM32_OPDS_INSDS_OFFSET,
               voice->guest_insds);
  store_u32_le(memory, voice->guest_opcode + WASM32_OPDS_INIT_OFFSET,
               descriptor->init_index);
  store_u32_le(memory, voice->guest_opcode + WASM32_OPDS_PERF_OFFSET,
               descriptor->perf_index);
  store_u32_le(memory, voice->guest_opcode + WASM32_OPDS_DEINIT_OFFSET,
               descriptor->deinit_index);

  for (i = 0; i < descriptor->argument_count; i++) {
    uint32_t capacity = descriptor->argument_types[i] == 'a'
                            ? ksmps * (uint32_t) sizeof(MYFLT)
                            : (uint32_t) sizeof(MYFLT);
    cursor += WASM32_VAR_TYPE_OFFSET;
    voice->arguments[i].address = cursor;
    voice->arguments[i].capacity = capacity;
    store_u32_le(memory, cursor - WASM32_VAR_TYPE_OFFSET,
                 argument_type_record(module, descriptor->argument_types[i]));
    store_u32_le(memory, voice->guest_opcode + WASM32_OPDS_SIZE + i * 4,
                 cursor);
    cursor = (cursor + capacity + 7u) & ~7u;
  }
  csound->UnlockMutex(module->mutex);
  return voice;

fail:
  release_voice_block(csound, module, voice->allocation);
  voice->allocation = NULL;
  csound->UnlockMutex(module->mutex);
  csound->Free(csound, voice);
  return NULL;
}

static void set_guest_block(WasmOpcodeInstance *opcode, WasmVoice *voice,
                            uint32_t ksmps, uint32_t offset, uint32_t early)
{
  WasmGuest *guest = &voice->module->guest;
  uint8_t *memory = wasmtime_memory_data(guest->context, &guest->memory);
  INSDS *insds = opcode->h.insdshead;
  store_f64_le(memory, voice->guest_insds + WASM32_INSDS_ESR_OFFSET,
               insds->esr);
  store_u32_le(memory, voice->guest_insds + WASM32_INSDS_KSMPS_OFFSET,
               ksmps);
  store_f64_le(memory, voice->guest_insds + WASM32_INSDS_EKR_OFFSET,
               insds->ekr);
  store_u32_le(memory,
               voice->guest_insds + WASM32_INSDS_KSMPS_OFFSET_OFFSET, offset);
  store_u32_le(memory,
               voice->guest_insds + WASM32_INSDS_KSMPS_NO_END_OFFSET, early);
}

static int32_t copy_inputs_to_guest(WasmOpcodeInstance *opcode,
                                    WasmVoice *voice, uint32_t offset,
                                    uint32_t sample_count)
{
  WasmOpcodeDescriptor *descriptor = opcode->descriptor;
  WasmGuest *guest = &voice->module->guest;
  uint8_t *memory = wasmtime_memory_data(guest->context, &guest->memory);
  uint32_t i;
  for (i = descriptor->output_count; i < descriptor->argument_count; i++) {
    MYFLT *argument = (MYFLT *) opcode->arguments[i];
    uint32_t bytes = descriptor->argument_types[i] == 'a'
                         ? sample_count * (uint32_t) sizeof(MYFLT)
                         : (uint32_t) sizeof(MYFLT);
    uint64_t guest_offset = descriptor->argument_types[i] == 'a'
                                ? (uint64_t) offset * sizeof(MYFLT)
                                : 0;
    if (argument == NULL || guest_offset > voice->arguments[i].capacity ||
        bytes > voice->arguments[i].capacity - guest_offset)
      return CSOUND_ERROR;
    if (descriptor->argument_types[i] == 'a') {
#ifdef WORDS_BIGENDIAN
      uint32_t sample;
      for (sample = 0; sample < sample_count; sample++)
        store_f64_le(memory,
                     voice->arguments[i].address + (offset + sample) * 8,
                     (double) argument[offset + sample]);
#else
      memcpy(memory + voice->arguments[i].address + offset * sizeof(MYFLT),
             argument + offset, bytes);
#endif
    }
    else
      store_f64_le(memory, voice->arguments[i].address, (double) *argument);
  }
  return CSOUND_SUCCESS;
}

static int32_t copy_outputs_from_guest(WasmOpcodeInstance *opcode,
                                       WasmVoice *voice, uint32_t offset,
                                       uint32_t sample_count)
{
  WasmOpcodeDescriptor *descriptor = opcode->descriptor;
  WasmGuest *guest = &voice->module->guest;
  const uint8_t *memory = wasmtime_memory_data(guest->context, &guest->memory);
  uint32_t i;
  for (i = 0; i < descriptor->output_count; i++) {
    MYFLT *argument = (MYFLT *) opcode->arguments[i];
    uint32_t bytes = descriptor->argument_types[i] == 'a'
                         ? sample_count * (uint32_t) sizeof(MYFLT)
                         : (uint32_t) sizeof(MYFLT);
    uint64_t guest_offset = descriptor->argument_types[i] == 'a'
                                ? (uint64_t) offset * sizeof(MYFLT)
                                : 0;
    if (argument == NULL || guest_offset > voice->arguments[i].capacity ||
        bytes > voice->arguments[i].capacity - guest_offset)
      return CSOUND_ERROR;
    if (descriptor->argument_types[i] == 'a') {
#ifdef WORDS_BIGENDIAN
      uint32_t sample;
      for (sample = 0; sample < sample_count; sample++)
        argument[offset + sample] = (MYFLT) load_f64_le(
            memory, voice->arguments[i].address + (offset + sample) * 8);
#else
      memcpy(argument + offset,
             memory + voice->arguments[i].address + offset * sizeof(MYFLT),
             bytes);
#endif
    }
    else
      *argument = (MYFLT) load_f64_le(memory, voice->arguments[i].address);
  }
  return CSOUND_SUCCESS;
}

static void clear_audio_outputs(WasmOpcodeInstance *opcode, uint32_t offset,
                                uint32_t sample_count)
{
  WasmOpcodeDescriptor *descriptor = opcode->descriptor;
  uint32_t i;
  for (i = 0; i < descriptor->output_count; i++) {
    if (descriptor->argument_types[i] == 'a' && opcode->arguments[i] != NULL)
      memset((MYFLT *) opcode->arguments[i] + offset, 0,
             sample_count * sizeof(MYFLT));
  }
}

static int32_t wasm_opcode_init(CSOUND *csound, WasmOpcodeInstance *opcode)
{
  WasmVoice *voice;
  WasmModule *module;
  uint32_t ksmps;
  uint32_t offset;
  uint32_t early;
  int32_t guest_result;

  opcode->descriptor = find_descriptor(csound, opcode);
  if (opcode->descriptor == NULL)
    return csound->InitError(csound, "Could not resolve Wasm opcode\n");
  if (opcode->voice != NULL) {
    destroy_voice(csound, opcode->voice);
    opcode->voice = NULL;
  }
  ksmps = opcode->h.insdshead->ksmps;
  offset = opcode->h.insdshead->ksmps_offset;
  early = opcode->h.insdshead->ksmps_no_end;
  voice = create_voice(csound, opcode->descriptor, ksmps);
  if (voice == NULL)
    return csound->InitError(csound, "Could not create Wasm opcode instance\n");
  opcode->voice = voice;
  module = voice->module;
  csound->LockMutex(module->mutex);
  set_guest_block(opcode, voice, ksmps, offset, early);
  if (copy_inputs_to_guest(opcode, voice, 0, ksmps) != CSOUND_SUCCESS) {
    csound->UnlockMutex(module->mutex);
    goto fail;
  }
  if (voice->has_init) {
    if (call_opcode_function(csound, voice, &voice->init,
                             "initialise Wasm opcode", 1,
                             &guest_result) != CSOUND_SUCCESS ||
        guest_result != OK) {
      csound->UnlockMutex(module->mutex);
      goto fail;
    }
  }
  voice->guest_initialized = 1;
  if (copy_outputs_from_guest(opcode, voice, 0, ksmps) != CSOUND_SUCCESS) {
    csound->UnlockMutex(module->mutex);
    goto fail;
  }
  csound->UnlockMutex(module->mutex);
  return OK;

fail:
  destroy_voice(csound, voice);
  opcode->voice = NULL;
  return csound->InitError(csound, "Wasm opcode initialisation failed\n");
}

static int32_t wasm_opcode_perf(CSOUND *csound, WasmOpcodeInstance *opcode)
{
  WasmVoice *voice = opcode->voice;
  WasmModule *module;
  uint32_t ksmps = opcode->h.insdshead->ksmps;
  uint32_t offset = opcode->h.insdshead->ksmps_offset;
  uint32_t early = opcode->h.insdshead->ksmps_no_end;
  uint32_t end = early <= ksmps ? ksmps - early : 0;
  uint32_t active = end > offset ? end - offset : 0;
  int32_t guest_result;

  clear_audio_outputs(opcode, 0, offset);
  if (end < ksmps)
    clear_audio_outputs(opcode, end, ksmps - end);
  if (voice == NULL || voice->failed || !voice->has_perf) {
    clear_audio_outputs(opcode, offset, active);
    return OK;
  }
  module = voice->module;
  csound->LockMutex(module->mutex);
  set_guest_block(opcode, voice, ksmps, offset, early);
  if (copy_inputs_to_guest(opcode, voice, offset, active) != CSOUND_SUCCESS)
    goto fail;
  if (call_opcode_function(csound, voice, &voice->perf,
                           "perform Wasm opcode", 1,
                           &guest_result) != CSOUND_SUCCESS)
    goto fail;
  if (guest_result != OK) {
    csound->UnlockMutex(module->mutex);
    clear_audio_outputs(opcode, offset, active);
    return guest_result;
  }
  if (copy_outputs_from_guest(opcode, voice, offset, active) != CSOUND_SUCCESS)
    goto fail;
  csound->UnlockMutex(module->mutex);
  return OK;

fail:
  voice->failed = 1;
  csound->UnlockMutex(module->mutex);
  clear_audio_outputs(opcode, offset, active);
  return csound->PerfError(csound, &opcode->h, "Wasm opcode failed\n");
}

static int32_t wasm_opcode_deinit(CSOUND *csound, WasmOpcodeInstance *opcode)
{
  destroy_voice(csound, opcode->voice);
  opcode->voice = NULL;
  return OK;
}

int32_t csoundLoadWasmOpcodeLibrary(CSOUND *csound, const char *path)
{
  WasmRegistry *registry;
  WasmModule *module;
  WasmModule **tail;
  WasmPluginMetadata metadata;
  uint8_t *bytes;
  size_t size;
  wasmtime_error_t *error;

  if (csound == NULL || path == NULL || path[0] == '\0')
    return CSOUND_ERROR;
  if (sizeof(MYFLT) != 8) {
    csound->ErrorMsg(csound,
                     "Wasm opcode libraries require a 64-bit MYFLT build\n");
    return CSOUND_ERROR;
  }
  registry = create_registry(csound);
  if (registry == NULL)
    return CSOUND_MEMORY;
  for (module = registry->modules; module != NULL; module = module->next) {
    if (strcmp(module->path, path) == 0)
      return CSOUND_SUCCESS;
  }
  bytes = read_module_file(path, &size);
  if (bytes == NULL) {
    csound->ErrorMsg(csound, "Could not read Wasm opcode library '%s'\n", path);
    return CSOUND_ERROR;
  }
  if (!read_opcode_wasm_metadata(bytes, size, &metadata)) {
    csound->ErrorMsg(csound,
                     "'%s' has no compatible OPCODE.WASM loader data\n", path);
    free(bytes);
    return CSOUND_ERROR;
  }
  module = (WasmModule *) csound->Calloc(csound, sizeof(*module));
  if (module == NULL) {
    free(bytes);
    return CSOUND_MEMORY;
  }
  module->path = csound->Strdup(csound, (char *) path);
  if (module->path == NULL) {
    csound->Free(csound, module);
    free(bytes);
    return CSOUND_MEMORY;
  }
  module->loader_memory_bytes = metadata.memory_bytes;
  module->loader_table_entries = metadata.table_entries;
  error = wasmtime_module_new(registry->engine, bytes, size, &module->code);
  free(bytes);
  if (error != NULL) {
    print_wasmtime_error(csound, "compile Wasm opcode library", error, NULL);
    destroy_module(csound, module);
    return CSOUND_ERROR;
  }
  if (discover_opcodes(csound, module) != CSOUND_SUCCESS) {
    destroy_module(csound, module);
    return CSOUND_ERROR;
  }
  module->mutex = csound->Create_Mutex(0);
  if (module->mutex == NULL) {
    destroy_module(csound, module);
    return CSOUND_MEMORY;
  }
  if (validate_unique_signatures(csound, registry, module) != CSOUND_SUCCESS) {
    destroy_module(csound, module);
    return CSOUND_ERROR;
  }
  tail = &registry->modules;
  while (*tail != NULL)
    tail = &(*tail)->next;
  *tail = module;
  return CSOUND_SUCCESS;
}

int32_t csoundInitWasmOpcodeLibraries(CSOUND *csound)
{
  WasmRegistry *registry = query_registry(csound);
  WasmModule *module;
  int32_t return_value = CSOUND_SUCCESS;
  uint32_t i;
  if (registry == NULL)
    return CSOUND_SUCCESS;

  for (module = registry->modules; module != NULL; module = module->next) {
    if (module->registered)
      continue;
    for (i = 0; i < module->opcode_count; i++) {
      WasmOpcodeDescriptor *descriptor = &module->opcodes[i];
      int32_t result = csound->AppendOpcode(
          csound, descriptor->name, sizeof(WasmOpcodeInstance),
          descriptor->flags, descriptor->outypes, descriptor->intypes,
          (SUBR) wasm_opcode_init,
          descriptor->perf_index == 0 ? NULL : (SUBR) wasm_opcode_perf,
          (SUBR) wasm_opcode_deinit);
      if (result == CSOUND_SUCCESS && descriptor->deprecated != 0)
        result = csound->Deprecate(csound, descriptor->name,
                                   descriptor->outypes, descriptor->intypes,
                                   descriptor->deprecated);
      if (result != CSOUND_SUCCESS)
        return_value = CSOUND_ERROR;
    }
    module->registered = 1;
  }
  return return_value;
}

int32_t csoundDestroyWasmOpcodeLibraries(CSOUND *csound)
{
  WasmRegistry **slot = (WasmRegistry **) csound->QueryGlobalVariable(
      csound, WASM_REGISTRY_NAME);
  WasmRegistry *registry;
  WasmModule *module;
  if (slot == NULL || *slot == NULL)
    return CSOUND_SUCCESS;
  registry = *slot;
  module = registry->modules;
  while (module != NULL) {
    WasmModule *next = module->next;
    destroy_module(csound, module);
    module = next;
  }
  if (registry->engine != NULL)
    wasm_engine_delete(registry->engine);
  csound->Free(csound, registry);
  *slot = NULL;
  csound->DestroyGlobalVariable(csound, WASM_REGISTRY_NAME);
  return CSOUND_SUCCESS;
}
