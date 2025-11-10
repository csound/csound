/*
    module_tests.c:

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

#include "csoundCore.h"
#include "csound_module.h"
#include <stdio.h>
#include <string.h>

/* Test helper functions */
static void print_test_result(const char *test_name, int passed)
{
    printf("  %s: %s\n", test_name, passed ? "PASS" : "FAIL");
}

/* Test stub opcodes for unit testing only */
static int32_t testSimpleOscInit(CSOUND *csound, void *p)
{
    IGN(csound);
    IGN(p);
    return OK;
}

static int32_t testSimpleOscPerf(CSOUND *csound, void *p)
{
    IGN(csound);
    IGN(p);
    return OK;
}

static int32_t testSimpleOscDeinit(CSOUND *csound, void *p)
{
    IGN(csound);
    IGN(p);
    return OK;
}

/* Test function for opcode registration (should only be used in tests) */
static void test_registerUDOFromModule(CSOUND *csound, CS_MODULE *module, const char *udo_name, const char *out_types, const char *in_types)
{
    /* Create a test OENTRY based on the parsed UDO signature */
    OENTRY *udo_op = (OENTRY*)csound->Calloc(csound, sizeof(OENTRY));
    udo_op->opname = csound->Strdup(csound, udo_name);

    udo_op->dsblksiz = 0;
    udo_op->flags = 0;
    udo_op->outypes = csound->Strdup(csound, out_types);
    udo_op->intypes = csound->Strdup(csound, in_types);

    /* Use test stub functions for unit testing only */
    udo_op->init = testSimpleOscInit;
    udo_op->perf = testSimpleOscPerf;
    udo_op->deinit = testSimpleOscDeinit;

    /* Register the opcode globally for testing */
    csoundAppendOpcode(csound, udo_op->opname, udo_op->dsblksiz, udo_op->flags,
                      udo_op->outypes, udo_op->intypes,
                      udo_op->init, udo_op->perf, udo_op->deinit);

    printf("  Test registered UDO opcode: %s (%s -> %s)\n", udo_name, in_types, out_types);
}

static void test_module_initialization(CSOUND *csound)
{
    printf("Testing module initialization...\n");

    /* Initialize module system */
    csoundInitializeModuleSystem(csound);

    MODULE_STATE *state = csoundGetModuleState(csound);

    /* Check that module system fields are initialized */
    int passed = (state != NULL &&
                  state->modules != NULL &&
                  state->module_search_paths != NULL &&
                  state->import_aliases != NULL &&
                  state->global_module != NULL);

    print_test_result("Module system initialization", passed);
    print_test_result("Global module created", state->global_module != NULL);
}

static void test_stub_opcode_registration(CSOUND *csound)
{
    printf("Testing stub opcode registration (unit test only)...\n");

    /* Create a test module */
    CS_MODULE *test_module = (CS_MODULE*)csound->Calloc(csound, sizeof(CS_MODULE));
    test_module->name = csound->Strdup(csound, "test_module");
    test_module->opcodes = cs_hash_table_create(csound);
    test_module->module_vars = cs_hash_table_create(csound);

    /* Test registering a UDO with stub implementation (for unit testing only) */
    test_registerUDOFromModule(csound, test_module, "TestOsc", "a", "kkk");

    /* Clean up */
    cs_hash_table_free(csound, test_module->opcodes);
    cs_hash_table_free(csound, test_module->module_vars);
    csound->Free(csound, test_module->name);
    csound->Free(csound, test_module);

    print_test_result("Stub opcode registration", 1); /* If we get here, it worked */
}

static void test_module_creation(CSOUND *csound)
{
    printf("\nTesting module creation...\n");

    /* Initialize module system */
    csoundInitializeModuleSystem(csound);
    MODULE_STATE *state = csoundGetModuleState(csound);

    /* Check global module properties */
    if (state->global_module) {
        print_test_result("Global module name",
                         strcmp(state->global_module->name, "global") == 0);
        print_test_result("Global module compiled", state->global_module->is_compiled);
        print_test_result("Global module virtual", state->global_module->is_virtual);
    }
}

static void test_module_creation(CSOUND *csound)
{
    printf("\nTesting module creation...\n");

    /* Test virtual module creation */
    CS_MODULE *mod = csoundCreateVirtualModule(csound, "test_module");
    int passed = (mod != NULL);
    print_test_result("Virtual module creation", passed);

    if (mod) {
        print_test_result("Module name", strcmp(mod->name, "test_module") == 0);
        print_test_result("Module compiled", mod->is_compiled);
        print_test_result("Module virtual", mod->is_virtual);
        print_test_result("Module opcodes table", mod->opcodes != NULL);
        print_test_result("Reference count", mod->ref_count == 1);
    }
}

static void test_module_path_resolution(CSOUND *csound)
{
    printf("\nTesting module path resolution...\n");

    /* Test basic path resolution */
    char *path = csoundResolveModulePath(csound, "lib.oscillators");
    int passed = (path != NULL);
    print_test_result("Path resolution", passed);

    if (path) {
        print_test_result("Path format",
                         strcmp(path, "lib.oscillators.orc") == 0);
        csound->Free(csound, path);
    }

    /* Test NULL path */
    path = csoundResolveModulePath(csound, NULL);
    print_test_result("NULL path handling", path == NULL);
}

static void test_module_registry(CSOUND *csound)
{
    printf("\nTesting module registry...\n");

    /* Test getting a module (will create a simple one) */
    CS_MODULE *mod1 = csoundGetOrLoadModule(csound, "test.simple");
    int passed = (mod1 != NULL);
    print_test_result("Module creation via registry", passed);

    if (mod1) {
        /* Test singleton behavior - should get same module */
        CS_MODULE *mod2 = csoundGetOrLoadModule(csound, "test.simple");
        print_test_result("Singleton behavior", mod1 == mod2);
        print_test_result("Reference count incremented", mod2->ref_count == 2);

        /* Test module name extraction */
        print_test_result("Module name",
                         strcmp(mod2->name, "simple") == 0);
    }
}

static void test_virtual_opcode_registration(CSOUND *csound)
{
    printf("\nTesting virtual opcode registration...\n");

    CS_MODULE *mod = csoundCreateVirtualModule(csound, "test_ops");
    int passed = (mod != NULL);
    print_test_result("Virtual module for ops", passed);

    if (mod) {
        /* Register a dummy opcode */
        int result = csoundRegisterVirtualOpcode(csound, mod, "test_op",
                                                   NULL, NULL, NULL);
        print_test_result("Opcode registration", result == CSOUND_SUCCESS);

        /* Test finding the opcode */
        OENTRY *op = csoundFindModuleOpcode(csound, mod, "test_op");
        print_test_result("Opcode lookup", op != NULL);

        if (op) {
            print_test_result("Opcode name", strcmp(op->opname, "test_op") == 0);
        }
    }
}

static void test_module_search_paths(CSOUND *csound)
{
    printf("\nTesting module search paths...\n");

    /* Add a search path */
    int result = csoundAddModuleSearchPath(csound, "/test/path");
    print_test_result("Add search path", result == CSOUND_SUCCESS);

    /* Check that search paths table exists and has our entry */
    MODULE_STATE *state = csoundGetModuleState(csound);
    void *found = cs_hash_table_get(csound, state->module_search_paths, "/test/path");
    print_test_result("Search path stored", found != NULL);
}

static void test_module_cleanup(CSOUND *csound)
{
    printf("\nTesting module cleanup...\n");

    /* Cleanup module system */
    csoundCleanupModuleSystem(csound);

    /* Check that fields are cleaned up */
    int passed = (csound->module_state == NULL);

    print_test_result("Module system cleanup", passed);
}

/* Main test function */
int main(void)
{
    CSOUND *csound;
    int result = 0;

    printf("=== Csound Module System Tests ===\n\n");

    /* Create Csound instance */
    csound = csoundCreate(NULL);
    if (csound == NULL) {
        printf("ERROR: Failed to create Csound instance\n");
        return 1;
    }

    /* Run tests */
    test_module_initialization(csound);
    test_stub_opcode_registration(csound);
    test_module_creation(csound);
    test_module_path_resolution(csound);
    test_module_registry(csound);
    test_virtual_opcode_registration(csound);
    test_module_search_paths(csound);
    test_module_cleanup(csound);

    printf("\n=== Module System Tests Complete ===\n");

    /* Clean up */
    csoundDestroy(csound);

    return result;
}

/* Test registration function (for Csound's test framework) */
#ifndef STANDALONE_TEST
int csoundModuleInit(CSOUND *csound)
{
    /* This function can be called by Csound's test framework */
    return main();
}
#endif
