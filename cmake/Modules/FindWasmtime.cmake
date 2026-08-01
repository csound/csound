# Find the compiler-capable Wasmtime C API.
#
# This module defines:
#
#   Wasmtime_FOUND
#   Wasmtime_VERSION
#   Wasmtime_INCLUDE_DIR
#   Wasmtime_LIBRARY
#   Wasmtime::wasmtime
#
# Set Wasmtime_ROOT, WASMTIME_ROOT, or WASMTIME_C_API_ROOT to the root of a
# Wasmtime C API release when it is not installed in a standard prefix.

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
include(FindPackageHandleStandardArgs)

set(_Wasmtime_ROOT_HINTS
    "${Wasmtime_ROOT}"
    "${WASMTIME_ROOT}"
    "${WASMTIME_C_API_ROOT}"
    "$ENV{Wasmtime_ROOT}"
    "$ENV{WASMTIME_ROOT}"
    "$ENV{WASMTIME_C_API_ROOT}"
)

find_path(Wasmtime_INCLUDE_DIR
    NAMES wasmtime.h
    HINTS ${_Wasmtime_ROOT_HINTS}
    PATH_SUFFIXES include
)

find_library(Wasmtime_LIBRARY
    NAMES wasmtime libwasmtime
    HINTS ${_Wasmtime_ROOT_HINTS}
    PATH_SUFFIXES lib
)

if(Wasmtime_INCLUDE_DIR)
    file(STRINGS "${Wasmtime_INCLUDE_DIR}/wasmtime.h" _Wasmtime_VERSION_LINE
        REGEX "^#define[ \t]+WASMTIME_VERSION[ \t]+\"[^\"]+\""
        LIMIT_COUNT 1
    )
    if(_Wasmtime_VERSION_LINE)
        string(REGEX REPLACE
            "^#define[ \t]+WASMTIME_VERSION[ \t]+\"([^\"]+)\".*"
            "\\1" Wasmtime_VERSION "${_Wasmtime_VERSION_LINE}"
        )
    endif()
endif()

# Reject AOT-only and older C API builds. Csound always compiles raw Wasm and
# uses newer store, reference, memory, and cache APIs.
if(Wasmtime_INCLUDE_DIR AND Wasmtime_LIBRARY)
    set(_Wasmtime_PROBE_KEY
        "${Wasmtime_INCLUDE_DIR}|${Wasmtime_LIBRARY}|${Wasmtime_VERSION}")
    if(NOT "${Wasmtime_PROBE_KEY}" STREQUAL "${_Wasmtime_PROBE_KEY}")
        unset(Wasmtime_HAS_REQUIRED_API CACHE)
    endif()
    set(Wasmtime_PROBE_KEY "${_Wasmtime_PROBE_KEY}" CACHE INTERNAL
        "Wasmtime C API probe inputs" FORCE)

    if(CMAKE_C_COMPILER_LOADED)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${Wasmtime_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_LIBRARIES "${Wasmtime_LIBRARY}")
        check_c_source_compiles([[
        #include <stdint.h>
        #include <wasmtime.h>

        #ifndef WASMTIME_FEATURE_COMPILER
        #error "Wasmtime was built without a compiler"
        #endif

        #ifndef WASMTIME_FEATURE_GC
        #error "Wasmtime was built without GC reference support"
        #endif

        #ifndef WASMTIME_FEATURE_CACHE
        #error "Wasmtime was built without its compilation cache"
        #endif

        int main(void)
        {
            wasm_config_t *config = wasm_config_new();
            wasm_engine_t *engine;
            wasmtime_store_t *store;
            wasmtime_context_t *context;
            wasmtime_module_t *module = NULL;
            wasmtime_instance_t instance;
            wasmtime_memory_t memory;
            wasmtime_table_t table;
            wasmtime_func_t function;
            wasmtime_extern_t item;
            wasmtime_val_t value = {.kind = WASMTIME_FUNCREF};
            wasm_importtype_vec_t imports;
            wasmtime_error_t *error;
            wasm_trap_t *trap = NULL;

            wasmtime_config_memory_reservation_set(config, 256 * 1024 * 1024);
            wasmtime_config_memory_may_move_set(config, false);
            wasmtime_config_memory_guard_size_set(config, 65536);
            wasmtime_config_memory_reservation_for_growth_set(config, 0);
            error = wasmtime_config_cache_config_load(config, NULL);
            engine = wasm_engine_new_with_config(config);
            store = wasmtime_store_new(engine, NULL, NULL);
            context = wasmtime_store_context(store);
            wasmtime_store_limiter(store, 256 * 1024 * 1024, 65536, 1, 1, 1);

            error = wasmtime_module_new(engine, NULL, 0, &module);
            wasmtime_module_imports(module, &imports);
            (void) wasmtime_memorytype_minimum(NULL);
            (void) wasmtime_memorytype_is64(NULL);
            (void) wasmtime_memorytype_isshared(NULL);
            error = wasmtime_memory_new(context, NULL, &memory);
            (void) wasmtime_memory_data(context, &memory);
            (void) wasmtime_memory_data_size(context, &memory);
            wasmtime_funcref_set_null(&value.of.funcref);
            error = wasmtime_table_new(context, NULL, &value, &table);
            error = wasmtime_table_set(context, &table, 0, &value);
            (void) wasmtime_table_get(context, &table, 0, &value);
            (void) wasmtime_funcref_is_null(&value.of.funcref);
            wasmtime_val_unroot(&value);
            wasmtime_func_new(context, NULL, NULL, NULL, NULL, &function);
            error = wasmtime_func_call(context, &function, NULL, 0, NULL, 0,
                                       &trap);
            error = wasmtime_instance_new(context, module, NULL, 0, &instance,
                                          &trap);
            (void) wasmtime_instance_export_get(context, &instance, NULL, 0,
                                                &item);
            wasmtime_extern_delete(&item);
            (void) wasmtime_caller_context(NULL);
            return error != NULL;
        }
        ]] Wasmtime_HAS_REQUIRED_API)
        cmake_pop_check_state()
    elseif(CMAKE_CXX_COMPILER_LOADED)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_INCLUDES "${Wasmtime_INCLUDE_DIR}")
        set(CMAKE_REQUIRED_LIBRARIES "${Wasmtime_LIBRARY}")
        check_cxx_source_compiles([[
            #include <wasmtime.h>

            #ifndef WASMTIME_FEATURE_COMPILER
            #error "Wasmtime was built without a compiler"
            #endif
            #ifndef WASMTIME_FEATURE_GC
            #error "Wasmtime was built without GC reference support"
            #endif
            #ifndef WASMTIME_FEATURE_CACHE
            #error "Wasmtime was built without its compilation cache"
            #endif

            int main()
            {
                wasm_config_t *config = wasm_config_new();
                wasm_engine_t *engine;
                wasmtime_module_t *module = nullptr;
                wasmtime_val_t value = {};
                wasmtime_error_t *error;
                wasmtime_config_memory_reservation_set(config,
                                                        256 * 1024 * 1024);
                wasmtime_config_memory_may_move_set(config, false);
                wasmtime_config_memory_guard_size_set(config, 65536);
                wasmtime_config_memory_reservation_for_growth_set(config, 0);
                error = wasmtime_config_cache_config_load(config, nullptr);
                engine = wasm_engine_new_with_config(config);
                error = wasmtime_module_new(engine, nullptr, 0, &module);
                wasmtime_store_limiter(nullptr, 256 * 1024 * 1024, 65536,
                                       1, 1, 1);
                (void) wasmtime_memorytype_minimum(nullptr);
                (void) wasmtime_memorytype_is64(nullptr);
                (void) wasmtime_memorytype_isshared(nullptr);
                wasmtime_val_unroot(&value);
                return error != nullptr;
            }
        ]] Wasmtime_HAS_REQUIRED_API)
        cmake_pop_check_state()
    else()
        set(Wasmtime_HAS_REQUIRED_API FALSE)
    endif()
endif()

find_package_handle_standard_args(Wasmtime
    REQUIRED_VARS
        Wasmtime_INCLUDE_DIR
        Wasmtime_LIBRARY
        Wasmtime_HAS_REQUIRED_API
    VERSION_VAR Wasmtime_VERSION
)

if(Wasmtime_FOUND AND NOT TARGET Wasmtime::wasmtime)
    add_library(Wasmtime::wasmtime UNKNOWN IMPORTED)
    set_target_properties(Wasmtime::wasmtime PROPERTIES
        IMPORTED_LOCATION "${Wasmtime_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Wasmtime_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(
    Wasmtime_INCLUDE_DIR
    Wasmtime_LIBRARY
    Wasmtime_HAS_REQUIRED_API
)

unset(_Wasmtime_ROOT_HINTS)
unset(_Wasmtime_VERSION_LINE)
unset(_Wasmtime_PROBE_KEY)
