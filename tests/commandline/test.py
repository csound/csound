#!/usr/bin/env python3

# Csound Test Suite
# By Steven Yi <stevenyi at gmail dot com>

import os
import sys
import threading
import logging
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from queue import Queue

##csoundExecutable = r"C:/Users/new/csound-csound6-git/csound.exe "
csoundExecutable = ""
sourceDirectory = "."
runtimeEnvironment = None

# Parallel execution configuration
enable_parallel = True  # Default to parallel execution
max_workers = None  # None = auto-detect based on CPU count
test_timeout = 300  # 5 minutes timeout per test
verbose_logging = False


# Setup logging for parallel execution
def setup_logging():
    """Setup logging configuration for parallel test execution."""
    level = logging.DEBUG if verbose_logging else logging.INFO

    logging.basicConfig(
        level=level,
        format="%(asctime)s - %(threadName)s - %(levelname)s - %(message)s",
        datefmt="%H:%M:%S",
    )
    return logging.getLogger("csound_tests")


logger = setup_logging()


class Test:
    def __init__(self, fileName, description="", expected=True):
        self.fileName = fileName
        self.description = description
        self.expected = expected


class TestResult:
    """Container for test execution results with ordering information."""

    def __init__(
        self, test_index, test_data, return_code, cs_output, execution_time, error=None
    ):
        self.test_index = test_index
        self.test_data = test_data
        self.return_code = return_code
        self.cs_output = cs_output
        self.execution_time = execution_time
        self.error = error
        self.filename = test_data[0]
        self.description = test_data[1]
        self.expected_result = (len(test_data) == 3) and 1 or 0

    @property
    def passed(self):
        """Check if test passed based on return code and expected result."""
        return (self.return_code == 0) == (self.expected_result == 0)

    @property
    def status_line(self):
        """Generate status line for test output."""
        if self.passed:
            return "[pass] - "
        else:
            return "[FAIL] - "

    def get_formatted_output(self, counter, verbose=False):
        """Get formatted output for this test result."""
        status = self.status_line
        output = f"{status}Test {counter}: {self.description} ({self.filename})\n"
        output += (
            f"\tReturn Code: {self.return_code}\tExpected: {self.expected_result}\n"
        )
        if self.error:
            output += f"\tError: {self.error}\n"
        if verbose and self.execution_time:
            output += f"\tExecution Time: {self.execution_time:.2f}s\n"
        return output


def execute_single_test(test_index, test_data, run_args, temp_file):
    """
    Execute a single Csound test and return the result.

    Args:
        test_index: Index of the test in the original test list
        test_data: Test data tuple [filename, description, optional_expected_result]
        run_args: Arguments to pass to csound
        temp_file: Temporary file for csound output

    Returns:
        TestResult object containing execution results
    """
    filename = test_data[0]
    desc = test_data[1]

    logger.debug(f"Starting test {test_index + 1}: {filename} - {desc}")
    start_time = time.time()

    try:
        # Prepare command based on OS
        if os.sep == "\\" or os.name == "nt":
            executable = (
                (csoundExecutable == "")
                and os.path.join("..", "csound.exe")
                or csoundExecutable
            )
        else:
            executable = (csoundExecutable == "") and "csound" or csoundExecutable

        if runtimeEnvironment:
            executable = f"{runtimeEnvironment} {executable}"

        # Use temp file for stderr capture (like the original implementation)
        command = f"{executable} {run_args} {sourceDirectory}/{filename} 2> {temp_file}"

        logger.debug(f"Executing command: {command}")

        # Execute the command with timeout using subprocess.run
        # (subprocess.run already handles WIFEXITED properly, unlike os.system)
        import subprocess

        try:
            result = subprocess.run(
                command,
                shell=True,
                timeout=test_timeout,
                capture_output=True,
                text=True,
            )
            return_code = result.returncode

            # For backward compatibility with os.system behavior,
            # if we were using os.system we would need WIFEXITED:
            # if hasattr(os, 'WIFEXITED') and os.WIFEXITED(return_code):
            #     return_code = os.WEXITSTATUS(return_code)
            # But subprocess.run already gives us the proper exit status

        except subprocess.TimeoutExpired:
            logger.error(f"Test {filename} timed out after {test_timeout} seconds")
            return TestResult(
                test_index,
                test_data,
                -1,
                "",
                test_timeout,
                error=f"Test timed out after {test_timeout} seconds",
            )

        # Read csound output from temp file (stderr)
        cs_output = ""
        try:
            with open(temp_file, "r") as f:
                cs_output = f.read()
        except IOError as e:
            logger.warning(f"Could not read temp file {temp_file}: {e}")

        # Also capture stdout if available (for complete test output like CI/CD systems)
        if result.stdout:
            cs_output += "\n[STDOUT CAPTURED]:\n" + result.stdout

        execution_time = time.time() - start_time

        logger.debug(
            f"Completed test {test_index + 1}: {filename} in {execution_time:.2f}s"
        )

        return TestResult(test_index, test_data, return_code, cs_output, execution_time)

    except Exception as e:
        execution_time = time.time() - start_time
        logger.error(f"Exception in test {filename}: {e}")
        return TestResult(
            test_index, test_data, -1, "", execution_time, error=f"Exception: {str(e)}"
        )


def run_tests_parallel(tests, run_args, max_workers=None, result_callback=None):
    """
    Run tests in parallel using ThreadPoolExecutor.

    Args:
        tests: List of test data tuples
        run_args: Arguments to pass to csound
        max_workers: Maximum number of worker threads (None for auto-detect)
        result_callback: Function to call when each test completes (for collecting results)

    Returns:
        List of TestResult objects ordered by original test index
    """
    if max_workers is None:
        import multiprocessing

        max_workers = min(multiprocessing.cpu_count(), len(tests))

    # Check if workers=1 for sequential execution
    if max_workers == 1:
        logger.info(f"Running {len(tests)} tests sequentially (workers=1)")
        return run_tests_sequential(tests, run_args, result_callback)

    logger.info(f"Running {len(tests)} tests in parallel with {max_workers} workers")

    # Create unique temp files for each worker to avoid conflicts
    temp_files = [
        f"csound_test_output_{threading.get_ident()}_{i}.txt"
        for i in range(max_workers)
    ]
    temp_file_queue = Queue()
    for temp_file in temp_files:
        temp_file_queue.put(temp_file)

    results = [None] * len(tests)

    def worker_with_temp_file(test_index_and_data):
        """Worker function that manages temp file allocation."""
        test_index, test_data = test_index_and_data
        temp_file = temp_file_queue.get()
        try:
            return execute_single_test(test_index, test_data, run_args, temp_file)
        finally:
            temp_file_queue.put(temp_file)

    # Execute tests in parallel
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        # Submit all tests
        future_to_index = {
            executor.submit(worker_with_temp_file, (i, test)): i
            for i, test in enumerate(tests)
        }

        # Collect results as they complete
        completed_count = 0
        for future in as_completed(future_to_index):
            original_index = future_to_index[future]
            try:
                result = future.result()
                results[original_index] = result
                completed_count += 1

                # Call result callback if provided (for collecting data, not immediate printing)
                if result_callback:
                    result_callback(result, completed_count)

                # Show simple progress indicator
                if completed_count % 10 == 0 or completed_count == len(tests):
                    logger.info(f"Completed {completed_count}/{len(tests)} tests")

            except Exception as e:
                logger.error(f"Test {original_index} generated an exception: {e}")
                # Create a failure result for this test
                test_data = tests[original_index]
                result = TestResult(
                    original_index,
                    test_data,
                    -1,
                    "",
                    0,
                    error=f"Execution exception: {str(e)}",
                )
                results[original_index] = result
                completed_count += 1

                # Call result callback for failed tests as well
                if result_callback:
                    result_callback(result, completed_count)

    # Clean up temp files
    for temp_file in temp_files:
        try:
            if os.path.exists(temp_file):
                os.remove(temp_file)
        except OSError:
            pass

    return results


def run_tests_sequential(tests, run_args, result_callback=None):
    """
    Run tests sequentially (original behavior).

    Args:
        tests: List of test data tuples
        run_args: Arguments to pass to csound
        result_callback: Function to call when each test completes (for collecting results)

    Returns:
        List of TestResult objects ordered by original test index
    """
    results = []
    tempfile = "csound_test_output.txt"

    for test_index, test_data in enumerate(tests):
        result = execute_single_test(test_index, test_data, run_args, tempfile)
        results.append(result)

        # Call result callback if provided
        if result_callback:
            result_callback(result, test_index + 1)

    # Clean up the temp file
    try:
        if os.path.exists(tempfile):
            os.remove(tempfile)
    except OSError:
        pass

    return results


def showHelp():
    message = """Csound Test Suite by Steven Yi<stevenyi@gmail.com>

    Runs tests and shows return values of tests. Results are written to
    results.txt file.

EXECUTION OPTIONS:
    --workers=<N>                  Number of worker threads (default: CPU count, 1 for sequential)
    --timeout=<seconds>            Timeout per test in seconds (default: 300)
    --verbose                      Enable verbose logging for execution

STANDARD OPTIONS:
    --csound-executable=<path>     Path to csound executable
    --opcode7dir64=<path>          Set OPCODE7DIR64 environment variable
    --source-dir=<path>            Source directory for tests (default: .)
    --runtime-environment=<env>    Runtime environment setup command
    --help                         Show this help message

EXAMPLES:
    ./test.py                                              # Parallel execution (default)
    ./test.py --workers=1                                  # Sequential execution
    ./test.py --workers=8                                  # Parallel with 8 workers
    ./test.py --timeout=60 --verbose                       # Custom timeout and logging

    """

    print(message)


def get_actual_workers():
    """Helper function to get actual worker count."""
    import multiprocessing

    return max_workers if max_workers else multiprocessing.cpu_count()


def runTest():
    runArgs = "-nd"  # "-Wdo test.wav"

    print("Testing with Csound")

    actual_workers = get_actual_workers()

    if actual_workers == 1:
        print("Running tests in SEQUENTIAL mode (workers=1)")
    else:
        print(f"Running tests in PARALLEL mode with {actual_workers} workers")

    print(f"Test timeout: {test_timeout} seconds per test")
    if verbose_logging:
        print("Verbose logging enabled")

    tests = [
        ["test1.csd", "Simple Test, Single Channel"],
        ["test2.csd", "Simple Test, 2 Channel"],
        ["test3.csd", "Simple Test, using i-rate variables, 2 Channel"],
        ["test4.csd", "Simple Test, using k-rate variables, 2 Channel"],
        ["test5.csd", "Simple Test, using global i-rate variables, 2 Channel"],
        ["test6.csd", "Testing Pfields"],
        ["test7.csd", "Testing expressions, no functions"],
        ["test8.csd", "Testing multi-part expressions, no functions"],
        ["test9.csd", "Unused Label (to test labels get parsed)"],
        ["test10.csd", "kgoto going to a label"],
        ["test11.csd", "if-kgoto going to a label, boolean expressions"],
        ["test12.csd", "Simple if-then statement"],
        ["test13.csd", "function call"],
        ["test14.csd", "polymorphic test, 0xffff (init)"],
        ["test15.csd", "pluck test, 0xffff (init)"],
        ["test16.csd", "Simple if-then with multiple statements in body"],
        ["test17.csd", "Simple if-then-else with multiple statements in body"],
        ["test18.csd", "if-then-elseif with no else block"],
        ["test19.csd", "if-elseif-else"],
        ["test20.csd", "if-elseif-else with inner if-elseif-else blocks"],
        ["test21.csd", "if-elseif-else with multiple elseif blocks"],
        ["test22.csd", "simple UDO"],
        ["test23.csd", "named instrument"],
        ##        ["test24.csd", "la_i opcodes"],
        ["test43.csd", "mixed numbered and named instruments"],
        ["test25.csd", "polymorphic test, 0xfffd (peak)"],
        ["test26.csd", "polymorphic test, 0xfffc (divz)"],
        ["test27.csd", "polymorphic test, 0xfffb (chnget)"],
        ["test28.csd", "label test"],
        ["test29.csd", "bit operations test"],
        ["test30.csd", "multi-numbered instrument test"],
        ["test31.csd", "i-rate conditional test"],
        ["test32.csd", "continuation lines test"],
        ["test33.csd", "using named instrument from score (testing score strings)"],
        ["test34.csd", "tertiary conditional expressions"],
        ["test35.csd", "test of passign"],
        ["test36.csd", "opcode with all input args optional (passign)"],
        ["test37.csd", "Testing in and out"],
        ["test38.csd", "Testing simple macro"],
        ["test39.csd", "Testing macro with argument"],
        ["test40.csd", "Testing i^j"],
        ["test41.csd", "if statement with = instead of =="],
        ["test42.csd", "extended string"],
        ["test44.csd", "expected failure with in-arg given to in opcode", 1],
        ["test45.csd", "if-goto with expression in boolean comparison"],
        ["test46.csd", "if-then with expression in boolean comparison"],
        ["test47.csd", "until loop and k[]"],
        ["test48.csd", "expected failure with variable used before defined", 1],
        ["test_instr0_call.csd", "testing ability to call instr 0"],
        ["test_udo_local_pool.csd", "test udos for separate local var pool"],
        ["test_ternary_expr.csd", "test ternary expr for backwards compatibility"],
        ["test_array_expr_opcall.csd", "test array expr in opcall"],
        ["test_shadowing_for_loop.csd", "test local shadowing of vars in for loop"],
        [
            "test_shadowing_implicit.csd",
            "test local shadowing of global vars for implicit types",
        ],
        ["test_opcode_with_opt_ins.csd", "test opcode with opt ins only"],
        ["test_fillarray_audio.csd", "test Arr:a[] = [sig:a]"],
        ["test_oversample.csd", "test oversampling in new-style UDO"],
        ["test_pvs_np2.csd", "test pvsanal/synth with np2 size"],
        ["test_instr_redefinition.csd", "allow instr redefinition"],
        ["test_instr0_labels.csd", "test labels in instr0 space"],
        ["test_string.csd", "test string assignment and printing"],
        ["test_sprintf.csd", "test string assignment and printing"],
        [
            "test_sprintf2.csd",
            "test string assignment and printing that causes reallocation",
        ],
        ["test_switch_statement.csd", "tests the new switch statement operator"],
        ["nested_strings.csd", "test nested strings works with schedule [issue #861]"],
        ["test_label_within_if_block.csd", "test label within if block"],
        ["test_labels.csd", "test labels with tab and space indentation"],
        ["test_newstyle_udo_optargs.csd", "test newstyle UDO optional args"],
        [
            "test_arrays.csd",
            "test k-array with single dimension, assignment to expression value",
        ],
        [
            "test_arrays2.csd",
            "test gk-array with single dimension, assignment to expression value",
        ],
        [
            "test_arrays3.csd",
            "test k-array with single dimension, assignment with number",
        ],
        [
            "test_arrays_multi.csd",
            "test multi-dimensionsl k-array, assigment to number and expression",
        ],
        ["test_arrays_string.csd", "test string-array"],
        ["test_arrays_string2.csd", "test simple string-array assignment"],
        [
            "test_arrays_static_init.csd",
            "test arrays initialized with static initializer (i.e. kvals = [0,1,2])",
        ],
        ["test_asig_as_array.csd", "test using a-sig with array get/set syntax"],
        [
            "test_arrays_negative_dimension_fail.csd",
            "test expected failure with negative dimension size and array",
            1,
        ],
        ["test_iarr_operators.csd", "test i[] operators"],
        ["test_booleans.csd", "tests using boolean data-types"],
        ["test_boolean_function.csd", "test boolean function in conditionals"],
        ["test_type_eq.csd", "test type equality operator"],
        ["test_audio_in.csd", "test the parsing of the 'in' operator as opcode"],
        [
            "test_empty_conditional_branches.csd",
            "tests that empty branches do not cause compiler issues",
        ],
        [
            "test_empty_instr.csd",
            "tests that empty instruments do not cause compiler issues",
        ],
        ["test_empty_udo.csd", "tests that empty UDOs do not cause compiler issues"],
        ["test_semantics_undefined_var.csd", "test undefined var", 1],
        ["test_opcall_expr.csd", "test expression in opcall"],
        ["test_invalid_expression.csd", "test expression", 1],
        ["test_invalid_ternary.csd", "test expression", 1],
        ["test_for_in.csd", "for in loop"],
        ["test_for_in2.csd", "for in loop (2nd form)"],
        ["test_opcode_as_function.csd", "test expression"],
        ["test_fsig_udo.csd", "UDO with f-sig arg"],
        ["test_karrays_udo.csd", "UDO with k[] arg"],
        ["test_arrays_addition.csd", "test array arithmetic (i.e. k[] + k[]"],
        ["test_arrays_fns.csd", "test functions on arrays (i.e. tabgen)", 1],
        ["test_polymorphic_udo.csd", "test polymorphic udo"],
        ["test_udo_a_array.csd", "test udo with a-array"],
        ["test_udo_2d_array.csd", "test udo with 2d-array"],
        ["test_udo_string_array_join.csd", "test udo with S[] arg returning S"],
        [
            "test_array_function_call.csd",
            "test synthesizing an array arg from a function-call",
        ],
        [
            "test_explicit_types.csd",
            "test typed identifiers (i.e. signals:a[], sigLeft:a)",
        ],
        ["test_parser3_opcall_ambiguities.csd", "test T_OPCALL ambiguities"],
        ["test_new_udo_syntax.csd", "test new-style UDO syntax"],
        [
            "test_new_udo_syntax_explicit_types.csd",
            "test new-style UDO syntax with explicit types",
        ],
        [
            "test_udo_array_args_implied_types.csd",
            "test new-style UDO with array args using implied types",
        ],
        [
            "test_multiple_return.csd",
            "test multiple return from express (i.. a1,a2 = xx())",
        ],
        [
            "test_array_operations.csd",
            "test multiple operations on multiple array types",
        ],
        [
            "prints_number_no_crash.csd",
            "test prints does not crash when given a number arguments",
            1,
        ],
        [
            "test_newlines_within_function_calls.csd",
            "test newlines allowed within function calls",
        ],
        ["test_comma_newline.csd", "test commas followed by newlines"],
        [
            "test_bool_with_explicit_type.csd",
            "test use of explicit type in bool expression",
        ],
        ["test_explicit_globals.csd", "test global declaration of explicit types"],
        [
            "test_fail_mismatched_types.csd",
            "syntax error on mismatched type declaration",
            1,
        ],
        [
            "test_fail_redefine.csd",
            "syntax error on redefinition of local var by global var in same context",
            1,
        ],
        ["test_var_redefine.csd", "test variable redefinition"],
        ["test_declare.csd", "test declare keyword (CS7)"],
        ["test_sub_str.csd", "test raw string embedded in raw string"],
        ["test_plusname.csd", "test +Name for instr name"],
        ["test_isactive.csd", "test isactive and isperforming"],
        [
            "test_unary_expressions.csd",
            "various unary operators in various expressions",
        ],
        ["test_opassign.csd", "test +=, ==, *= and /="],
        ["testnewline.csd", "test newline in statements"],
        ["test_string_in_event.csd", "test multiple strings in realtime event"],
        ["testmidichannels.csd", "test use of mapped multiport channels"],
        ["test_instr_type.csd", "test instr type and variables"],
        ["test_delete_instr.csd", "test creating and deleting instr"],
        ["test_create_instr.csd", "testing creating and scheduling instr"],
        ["test_instance_type.csd", "testing instance type"],
        ["test_play_opcode.csd", "testing play opcode"],
        ["test_create_init_perf_delete.csd", "testing new instance opcodes"],
        ["test_complex_numbers.csd", "testing complex number operations"],
        [
            "test_schedule_named_instance.csd",
            "testing schedule with named instr instance",
        ],
        [
            "test_instr_type_var_new_compilation.csd",
            "testing schedule of named instr in new compilations",
        ],
        ["test_redef_array.csd", "test redef of OpcodeDef by an array"],
        ["test_ambiguous_opcall.csd", "test ambiguous opcall examples"],
        ["test_opcode_type.csd", "tests opcode type"],
        ["test_opcode_obj_loop.csd", "tests array of opcode objects in loops"],
        ["test_sa.csd", "test sample accurate mode"],
        ["test_overload_selection.csd", "test wrong annotation case"],
        ["test_unschedule.csd", "test unscheduling events"],
        ["diskin_excess_channels.csd", "test sample accurate mode"],
        ["test_midifile_ops.csd", "testing midifile opcodes"],
        ["test_csound_object.csd", "test Csound object opcodes"],
        ["test_true_false.csd", "testing true/false booleans"],
        [
            "test_break_continue.csd",
            "testing break/continue statements in while/until/for loops",
        ],
        [
            "test_break_outside_loop_fails.csd",
            "testing break outside loop gives parser error",
            1,
        ],
        [
            "test_continue_outside_loop_fails.csd",
            "testing continue outside loop gives parser error",
            1,
        ],
        [
            "test_osc_server.csd",
            "test OSC in udp server",
            0,
            "-odac -d -+rtaudio=dummy",
        ],
        ["test_named_instr_ramps.csd", "test named instrument ramps"],
        ["test_gen01.csd", "testing GEN01 importing files"],
        ["test_raw_strings.csd", "test new-style raw strings"],
        ["test_min_max_values.csd", "test MIN_VALUE and MAX_VALUE math constants"],
        ["test_all_math_constants.csd", "test all builtin math constant macros"],
        [
            "test_op_precedence.csd",
            "test bitwise operator precedence vs equality",
        ],
        [
            "test_keyword_spacing.csd",
            "test keyword spacing (if(, elseif(, etc.)",
        ],
    ]

    arrayTests = [
        ["arrays/arrays_i_local.csd", "local i[]"],
        ["arrays/arrays_i_global.csd", "global i[]"],
        ["arrays/arrays_k_local.csd", "local k[]"],
        ["arrays/arrays_k_global.csd", "global k[]"],
        ["arrays/arrays_a_local.csd", "local a[]"],
        ["arrays/arrays_a_global.csd", "global a[]"],
        ["arrays/arrays_S_local.csd", "local S[]"],
        ["arrays/arrays_S_global.csd", "global S[]"],
        [
            "arrays/array_get_inline.csd",
            "tests parsing and eval of inline array[getters]",
        ],
        ["arrays/arrays_for_loop.csd", "tests for loops over array types"],
        ["arrays/test_redef_fail.csd", "fail on redefinition of variable by array", 1],
        ["arrays/array_copy.csd", "test for =.generic copy on k-rate only"],
        ["complex_array_test.csd", "testing complex array ops"],
        ["fft_array_test.csd", "testing complex fft array ops"],
        ["rfft_array_test.csd", "testing complex rfft array ops"],
        ["test_k_i_array_args.csd", "testing k[] in-arg with i[] var"],
        ["test_gen_array.csd", "testing genarray shorthand"],
        ["test_array_scalar_set.csd", "testing array scalar setting"],
        ["test_array_annotation.csd", "testing array type annotation for opcodes"],
        ["test_slice_array.csd", "testing slice shorthand"],
        ["arrays/array_a_arithm.csd", "test audio array arithmetic operations"],
        ["arrays/test_array_copy.csd", "test array copy operations"],
        ["test_arrays_constant_index.csd", "test arrays with constant index"],
        ["test_arrays_in_udo.csd", "test arrays in UDO"],
        ["test_array_in_expression.csd", "test expressions involving arrays"],
    ]

    structTests = [
        ["structs/test_structs.csd", "basic struct test"],
        ["structs/test_sub_structs.csd", "read/write to struct member of struct"],
        ["structs/test_struct_arrays.csd", "arrays of structs"],
        [
            "structs/test_current_limitations.csd",
            "test current struct implementation limitations",
        ],
        ["structs/test_nested_structs.csd", "test nested struct types"],
        ["structs/test_nested_types.csd", "test nested type definitions"],
        [
            "structs/test_simple_struct_assignment.csd",
            "test struct-to-struct assignment",
        ],
        [
            "structs/test_single_member_init.csd",
            "test single member struct initialization",
        ],
        ["structs/test_struct_arrays_2.csd", "test struct-to-struct references"],
        ["structs/test_struct_arrays_recursive.csd", "test recursive struct arrays"],
        ["structs/test_struct_assign.csd", "test struct assignment"],
        ["structs/test_struct_debug.csd", "test struct debugging"],
        ["structs/test_struct_member_access.csd", "test struct member access"],
        ["structs/test_struct_print_simple.csd", "test simple struct printing"],
        ["structs/test_struct_print_simple2.csd", "test simple struct printing 2"],
        ["structs/test_structs_2.csd", "test structs 2"],
        [
            "structs/test_nonexistent_member.csd",
            "test what breaking example with structs",
            1,
        ],
    ]

    udoTests = [
        ["udo/fail_no_xin.csd", "fail due to no xin", 1],
        ["udo/fail_no_xout.csd", "fail due to no xout", 1],
        ["udo/fail_invalid_xin.csd", "fail due to invalid xin", 1],
        ["udo/fail_invalid_xout.csd", "fail due to invalid xout", 1],
        ["udo/test_udo_xout_const.csd", "Constants as xout inputs work"],
        ["udo/pass_by_ref.csd", "Pass-by-ref works with new-style UDOs"],
        ["udo/test_args_in.csd", "Pass-by-ref connects args correctly."],
        ["udo/test_K_type.csd", "K-type arguments work with pass-by-ref"],
        [
            "udo/test_perf_stub_no_perf_chain.csd",
            "init-only UDO call in perf chain does not error",
        ],
        ["udo/crashing_test.csd", "test for UDO crashing", 1],
        ["udo/test_udo_array_set.csd", "test UDO array setting"],
        [
            "test_udo_optional_after_instr.csd",
            "test new-style UDO optional args defined after instr",
        ],
    ]

    maxallocTests = [
        ["test_maxalloc_turnoff_lt_0.csd", "Test maxalloc opcode less than 0", 1],
        ["test_maxalloc_turnoff_gt_2.csd", "Test maxalloc opcode greater than 2", 1],
        ["test_maxalloc_turnoff_default.csd", "Test maxalloc opcode defaults 0"],
        ["test_maxalloc_turnoff_eq_0.csd", "Test maxalloc opcode value of 0"],
        ["test_maxalloc_turnoff_eq_1.csd", "Test maxalloc opcode value of 1"],
        ["test_maxalloc_turnoff_eq_2.csd", "Test maxalloc opcode value of 2"],
    ]

    pfieldTests = [
        [
            "test_pfields_array.csd",
            "Test dynamic allocation of pfields, schedule and ftgen",
        ],
        ["test_schedule.csd", "Test pfields on all forms of schedule"],
        [
            "test_schedule_instr.csd",
            "Test pfields on all forms of schedule with instance",
        ],
        ["test_event_pfields.csd", "Test pfields on all forms of event"],
        ["test_recursive_schedule.csd", "Test recursive events"],
        ["test_file_table.csd", "Test ftgen gen01 file input"],
        ["test_midifile.csd", "Test midi file input (-F)"],
    ]

    tests += arrayTests
    tests += structTests
    tests += udoTests
    tests += maxallocTests
    tests += pfieldTests

    output = ""

    retVals = []

    testPass = 0
    testFail = 0
    testFailMessages = ""

    # Function to collect results without immediate printing
    def collect_result(result, completed_count):
        """Collect test results without immediate printing."""
        counter = result.test_index + 1
        retVal = result.return_code
        csOutput = result.cs_output

        # Count pass/fail
        nonlocal testPass, testFail, testFailMessages, output, retVals
        if result.passed:
            testPass += 1
        else:
            testFail += 1

        # Generate formatted output for later display (using verbose flag)
        formatted_output = result.get_formatted_output(counter, verbose_logging)

        # Show test output on failure immediately (like CI/CD systems)
        if not result.passed and csOutput:
            print(f"[FAILED] Test {counter}: {result.description} ({result.filename})")
            print(f"[TEST OUTPUT for {result.filename}]:")
            print("-" * 60)
            print(csOutput.strip())
            print("-" * 60)
            print()  # Add spacing

        # Add to failure messages if needed
        if not result.passed:
            testFailMessages += formatted_output
            if csOutput:
                testFailMessages += f"[TEST OUTPUT]:\n{csOutput}\n"

        # Generate detailed output for results file
        output += "%s\n" % ("=" * 80)
        output += "Test %i: %s (%s)\nReturn Code: %i\n" % (
            counter,
            result.description,
            result.filename,
            retVal,
        )
        if result.error:
            output += f"Error: {result.error}\n"
        if result.execution_time:
            output += f"Execution Time: {result.execution_time:.2f}s\n"
        output += "%s\n\n" % ("=" * 80)
        output += csOutput
        output += "\n\n"

        # Maintain backward compatibility for retVals
        retVals.append(result.test_data + [retVal, csOutput])

        # Return formatted output for later display
        return formatted_output

    # Execute tests and collect results
    start_time = time.time()
    results = run_tests_parallel(tests, runArgs, max_workers, collect_result)
    total_execution_time = time.time() - start_time

    logger.info(f"All tests completed in {total_execution_time:.2f} seconds")

    # Display results in ORIGINAL ORDER (this is the key fix!)
    print("\n%s\n" % ("=" * 80))
    print("TEST RESULTS (in original order):\n")

    for i, result in enumerate(results):
        if result:  # Check if result exists (should always be true)
            counter = i + 1
            formatted_output = result.get_formatted_output(counter, verbose_logging)
            print(formatted_output)

    print("%s\n\n" % ("=" * 80))
    print("Tests Passed: %i\nTests Failed: %i\n" % (testPass, testFail))

    if testFail > 0:
        print("[FAILED TESTS]\n\n%s" % testFailMessages)

    # Log execution summary
    actual_workers = get_actual_workers()
    avg_time_per_test = total_execution_time / len(tests) if tests else 0

    if actual_workers == 1:
        logger.info(f"Sequential execution completed in {total_execution_time:.2f}s")
    else:
        logger.info(f"Parallel execution with {actual_workers} workers")
        logger.info(f"Average time per test: {avg_time_per_test:.2f}s")

    f = open("results.txt", "w")
    f.write(output)
    f.flush()
    f.close()

    return testFail


if __name__ == "__main__":
    if len(sys.argv) > 1:
        for arg in sys.argv:
            if arg == "--help":
                showHelp()
                sys.exit(0)
            elif arg == "--verbose":
                verbose_logging = True
                # Reconfigure logging with verbose level
                logger.setLevel(logging.DEBUG)
                for handler in logger.handlers:
                    handler.setLevel(logging.DEBUG)
            elif arg.startswith("--csound-executable="):
                csoundExecutable = arg[20:]
                print(csoundExecutable)
            elif arg.startswith("--opcode7dir64="):
                os.environ["OPCODE7DIR64"] = arg[15:]
                print(os.environ["OPCODE7DIR64"])
            elif arg.startswith("--source-dir="):
                sourceDirectory = arg[13:]
            elif arg.startswith("--runtime-environment="):
                runtimeEnvironment = arg[22:]
            elif arg.startswith("--workers="):
                try:
                    max_workers = int(arg[10:])
                    if max_workers <= 0:
                        print("Error: Worker count must be greater than 0")
                        sys.exit(1)
                except ValueError:
                    print(
                        "Error: Invalid worker count. Use --workers=<N> where N is a positive integer."
                    )
                    sys.exit(1)
            elif arg.startswith("--timeout="):
                try:
                    test_timeout = int(arg[10:])
                    if test_timeout <= 0:
                        print("Error: Timeout must be greater than 0 seconds")
                        sys.exit(1)
                except ValueError:
                    print(
                        "Error: Invalid timeout. Use --timeout=<N> where N is a positive integer."
                    )
                    sys.exit(1)

    results = runTest()
    sys.exit(results)
