#!/usr/bin/python3

# Csound Module System Import Tests
# This file defines all import-related tests for the Csound module system.
# Import this file and add importTests to the main tests list in test.py

import_tests = [
    # Baseline tests (no imports)
    ["imports/test_baseline_local_udo.csd", "Local UDO baseline - no imports"],
    ["imports/test_local_udo_only.csd", "Local UDO only - no imports"],

    # Basic global variable tests
    ["imports/test_basic_globals.csd", "Basic global variables"],
    ["imports/test_simple_global.csd", "Simple global variables"],
    ["imports/test_comprehensive_globals.csd", "Comprehensive global variable tests"],

    # Minimal import tests
    ["imports/test_minimal.csd", "Minimal import test"],
    ["imports/test_import_minimal.csd", "Minimal import test variant"],

    # Simple import functionality
    ["imports/test_simple.csd", "Basic UDO import and execution"],
    ["imports/test_simple_import.csd", "Simple import test"],

    # Compilation tests
    ["imports/test_compilation_only.csd", "Compilation without execution"],
    ["imports/test_compile_only_no_run.csd", "Compile-only mode"],
    ["imports/test_header_only.csd", "Header-only imports"],

    # UDO import tests
    ["imports/test_import.csd", "UDO imports with correct type matching"],
    ["imports/test_multiple_udos.csd", "Multiple UDO imports"],

    # Module variable tests
    ["imports/test_module_variable.csd", "Module variable access (g-prefix AND @global)"],

    # Namespace tests
    ["imports/test_namespace_isolation.csd", "UDO namespace isolation"],

    # Import syntax variants
    ["imports/test_from_import.csd", "From import syntax"],

    # Import with alias (import "x.orc" as name) tests
    ["imports/test_import_as_qualified.csd", "Import with alias - qualified variable access"],
    ["imports/test_same_module_different_aliases.csd", "Same module imported with different aliases"],
    ["imports/test_different_modules_same_alias.csd", "Different modules with same alias (shadowing)"],
    ["imports/test_nested_module_aliases.csd", "Nested module imports with aliases"],

    # Complex tests
    ["imports/test_nested.csd", "Nested module imports"]
]