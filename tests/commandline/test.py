#!/usr/bin/env python3

# Csound Test Suite
# By Steven Yi <stevenyi at gmail dot com>

import logging
import locale
import os
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import time
from contextlib import contextmanager
from concurrent.futures import ThreadPoolExecutor, as_completed

##csoundExecutable = r"C:/Users/new/csound-csound6-git/csound.exe "
csoundExecutable = ""
sourceDirectory = "."
runtimeExecutable = None
runtimeArguments = []
expectedFailures = set()
outputEncoding = locale.getpreferredencoding(False) or "utf-8"

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
        self.expected_result = (
            1 if len(test_data) == 3 or self.filename in expectedFailures else 0
        )
        self.expectation = (
            test_data[3]
            if len(test_data) >= 4 and isinstance(test_data[3], dict)
            else None
        )
        if self.expectation is not None:
            self.expected_result = self.expectation.get("return_code", 0)
        else:
            self.expected_result = (len(test_data) == 3) and 1 or 0

    @staticmethod
    def _patterns(value):
        """Normalize one expected-output pattern or a list of patterns."""
        if value is None:
            return []
        return value if isinstance(value, list) else [value]

    @property
    def validation_error(self):
        """Explain why a structured return-code/output expectation failed."""
        if self.expectation is None:
            return None

        if self.return_code != self.expected_result:
            return (
                f"return code {self.return_code}, expected exactly "
                f"{self.expected_result}"
            )

        for pattern in self._patterns(self.expectation.get("output_contains")):
            if pattern not in self.cs_output:
                return f"required diagnostic not found: {pattern!r}"

        for pattern in self._patterns(self.expectation.get("output_excludes")):
            if pattern in self.cs_output:
                return f"forbidden crash diagnostic found: {pattern!r}"

        return None

    @property
    def passed(self):
        """Check if test passed based on return code and expected result."""
        if self.error is not None:
            return False
        if self.expectation is not None:
            return self.validation_error is None
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
        if self.validation_error:
            output += f"\tValidation: {self.validation_error}\n"
        if verbose and self.execution_time:
            output += f"\tExecution Time: {self.execution_time:.2f}s\n"
        return output


def decode_process_output(data):
    """Decode subprocess bytes without allowing logging text to fail a test."""
    if not data:
        return ""
    if isinstance(data, str):
        return data
    return data.decode(outputEncoding, errors="replace")


def collect_process_output(stderr_file, stdout):
    """Collect child stderr and stdout in the format used by test reports."""
    stderr_file.flush()
    stderr_file.seek(0)
    output = decode_process_output(stderr_file.read())
    stdout_text = decode_process_output(stdout)
    if stdout_text:
        output += "\n[STDOUT CAPTURED]:\n" + stdout_text
    return output


def resolve_command_path(command):
    """Resolve relative executable paths before changing the child cwd."""
    if os.path.isabs(command) or not os.path.dirname(command):
        return command
    return os.path.abspath(command)


def split_command_args(args):
    """Normalize optional per-test arguments into subprocess argv entries."""
    if not args:
        return []
    if isinstance(args, str):
        return shlex.split(args)
    return list(args)


def make_tree_writable(root):
    """Give the current user write access throughout a copied test tree."""
    for current_root, directory_names, file_names in os.walk(root):
        paths = [current_root]
        paths.extend(
            os.path.join(current_root, name)
            for name in directory_names + file_names
        )
        for path in paths:
            mode = stat.S_IMODE(os.stat(path).st_mode)
            os.chmod(path, mode | stat.S_IWUSR)


@contextmanager
def runtime_working_directory():
    """Provide runtimes with a writable copy mounted at their current cwd."""
    if not runtimeExecutable:
        yield None
        return

    with tempfile.TemporaryDirectory(prefix="csound-commandline-tests-") as root:
        working_directory = os.path.join(root, "tests")
        shutil.copytree(sourceDirectory, working_directory)
        make_tree_writable(working_directory)
        yield working_directory


def execute_single_test(test_index, test_data, run_args, working_directory=None):
    """
    Execute a single Csound test and return the result.

    Args:
        test_index: Index of the test in the original test list
        test_data: Test data tuple [filename, description,
            optional_expected_result, optional_run_args,
            optional_application_args, optional_stack_limit_kb]
        run_args: Arguments to pass to csound
    Returns:
        TestResult object containing execution results
    """
    filename = test_data[0]
    desc = test_data[1]
    test_run_args = test_data[3] if len(test_data) >= 4 else run_args
    application_args = test_data[4] if len(test_data) >= 5 else ""
    stack_limit_kb = test_data[5] if len(test_data) >= 6 else None

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

        # Keep the runtime as structured argv so paths containing spaces work on
        # both POSIX and Windows. Runtime options precede the module path.
        command = []
        if runtimeExecutable:
            command.append(resolve_command_path(runtimeExecutable))
            command.extend(runtimeArguments)
            # A runtime consumes a module file, not a PATH-resolved command.
            command.append(os.path.abspath(executable))
        else:
            command.append(resolve_command_path(executable))
        command.extend(split_command_args(test_run_args))
        if working_directory:
            # Test names already use portable forward slashes, as required by a
            # WASI guest regardless of the host platform.
            command.append(filename)
        else:
            command.append(os.path.join(sourceDirectory, filename))
        command.extend(split_command_args(application_args))
        if stack_limit_kb is not None and os.name == "posix":
            command = [
                "/bin/sh",
                "-c",
                'ulimit -s "$1" && shift && exec "$@"',
                "csound-test",
                str(int(stack_limit_kb)),
                *command,
            ]

        logger.debug(f"Executing command: {command}")

        # An anonymous OS temporary file avoids collisions between concurrent
        # test-suite processes and does not require a writable working directory.
        with tempfile.TemporaryFile(mode="w+b") as stderr_file:
            try:
                result = subprocess.run(
                    command,
                    timeout=test_timeout,
                    stdout=subprocess.PIPE,
                    stderr=stderr_file,
                    cwd=working_directory,
                )
                return_code = result.returncode

            except subprocess.TimeoutExpired as error:
                logger.error(
                    f"Test {filename} timed out after {test_timeout} seconds"
                )
                cs_output = collect_process_output(stderr_file, error.stdout)
                return TestResult(
                    test_index,
                    test_data,
                    -1,
                    cs_output,
                    test_timeout,
                    error=f"Test timed out after {test_timeout} seconds",
                )

            cs_output = collect_process_output(stderr_file, result.stdout)

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


def run_tests_parallel(
    tests,
    run_args,
    max_workers=None,
    result_callback=None,
    working_directory=None,
):
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
        return run_tests_sequential(
            tests, run_args, result_callback, working_directory
        )

    logger.info(f"Running {len(tests)} tests in parallel with {max_workers} workers")

    results = [None] * len(tests)

    def worker(test_index_and_data):
        test_index, test_data = test_index_and_data
        return execute_single_test(
            test_index, test_data, run_args, working_directory
        )

    # Execute tests in parallel
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        # Submit all tests
        future_to_index = {
            executor.submit(worker, (i, test)): i
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

    return results


def run_tests_sequential(
    tests, run_args, result_callback=None, working_directory=None
):
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
    for test_index, test_data in enumerate(tests):
        result = execute_single_test(
            test_index, test_data, run_args, working_directory
        )
        results.append(result)

        # Call result callback if provided
        if result_callback:
            result_callback(result, test_index + 1)

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
    --runtime-executable=<path>    Runtime executable placed before the Csound module
    --runtime-arg=<arg>            Runtime argument; repeat once per argument
    --expected-failure=<file>      Treat a nonzero result as expected for this test
    --runtime-environment=<path>   Deprecated executable-path-only alias
    --help                         Show this help message

EXAMPLES:
    ./test.py                                              # Parallel execution (default)
    ./test.py --workers=1                                  # Sequential execution
    ./test.py --workers=8                                  # Parallel with 8 workers
    ./test.py --timeout=60 --verbose                       # Custom timeout and logging
    ./test.py --runtime-executable=wasmtime --runtime-arg=run \
      --runtime-arg=--dir=. --csound-executable=/path/to/csound-cli.wasm

    """

    print(message)


def get_actual_workers():
    """Helper function to get actual worker count."""
    import multiprocessing

    return max_workers if max_workers else multiprocessing.cpu_count()


def runTest():
    runArgs = ["-nd"]  # ["-Wdo", "test.wav"]

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
        ["test_local_ksmps_global_fail.csd", "test failing use of global var with local ksmps",  1],
        ["test_local_ksmps_global_struct_k.csd", "allow global k-rate struct member with local ksmps"],
        ["test_local_ksmps_global_struct_a_fail.csd", "reject global a-rate struct member with local ksmps", 1],
        ["test_local_ksmps_global_struct_copy_fail.csd", "reject global struct containing audio with local ksmps", 1],
        ["test_schedwhen.csd", "schedwhen opcode"],
        ["test_parse_error_unary.csd", "expected failure: unary parse error", 1],
        ["test_parse_error_unary_not.csd", "expected failure: unary ! parse error", 1],
        ["test_parse_error_unary_minus.csd", "expected failure: unary - parse error", 1],
        ["test_parse_error_unary_plus.csd", "expected failure: unary + parse error", 1],
        ["test_parse_error_binary.csd", "expected failure: binary parse error", 1],
        ["test_parse_error_div.csd", "expected failure: binary / parse error", 1],
        ["test_parse_error_pow.csd", "expected failure: binary ^ parse error", 1],
        ["test_parse_error_mod.csd", "expected failure: binary % parse error", 1],
        ["test_parse_error_bitor.csd", "expected failure: binary | parse error", 1],
        ["test_parse_error_bitand.csd", "expected failure: binary & parse error", 1],
        ["test_parse_error_bitxor.csd", "expected failure: binary # parse error", 1],
        ["test_parse_error_shift_left.csd", "expected failure: binary << parse error", 1],
        ["test_parse_error_shift_right.csd", "expected failure: binary >> parse error", 1],
        ["test_parse_error_lt.csd", "expected failure: binary < parse error", 1],
        ["test_parse_error_gt.csd", "expected failure: binary > parse error", 1],
        ["test_parse_error_le.csd", "expected failure: binary <= parse error", 1],
        ["test_parse_error_ge.csd", "expected failure: binary >= parse error", 1],
        ["test_parse_error_neq.csd", "expected failure: binary != parse error", 1],
        ["test_parse_error_eqeq.csd", "expected failure: binary == parse error", 1],
        ["test_parse_error_assign_expr.csd", "expected failure: binary = parse error", 1],
        ["test_parse_error_and.csd", "expected failure: binary && parse error", 1],
        ["test_parse_error_or.csd", "expected failure: binary || parse error", 1],
        ["test_parse_error_ternary.csd", "expected failure: ternary parse error", 1],
        ["test_parse_error_instr_missing_id.csd", "expected failure: instr missing id", 1],
        ["test_parse_error_opcode_missing_name.csd", "expected failure: opcode missing name", 1],
        ["test_parse_error_opcode_missing_endop.csd", "expected failure: opcode missing endop", 1],
        ["test_parse_error_udo_missing_inargs.csd", "expected failure: udo missing inargs", 1],
        ["test_parse_error_udo_missing_commas.csd", "expected failure: udo missing commas", 1],
        ["test_parse_error_udo_missing_arglist.csd", "expected failure: udo missing arg list", 1],
        ["test_gen28_malformed.csd", "reject malformed GEN28 trajectory", 1],
        ["test_gen28_truncated.csd", "reject truncated GEN28 trajectory", 1],
        ["test_gen28_terminal_point.csd", "keep GEN28 terminal point values"],
        ["test_gen44_matrix_valid.csd", "load a valid GEN44 matrix"],
        ["test_gen44_matrix_resize.csd", "resize a short GEN44 table"],
        ["test_gen44_matrix_oversized.csd", "reject an oversized GEN44 matrix", 1],
        ["test_gen44_matrix_negative.csd", "reject a negative GEN44 matrix size", 1],
        ["test_gen44_matrix_zero.csd", "reject a zero GEN44 matrix size", 1],
        ["test_gen44_matrix_malformed.csd", "reject a malformed GEN44 matrix size", 1],
        ["test_gen44_matrix_missing_close.csd", "reject a GEN44 header without >", 1],
        ["test_gen49_defer.csd", "test GEN49 deferred length"],
        [
            "test_gen41_gen42_bounds.csd",
            "test bounded GEN41 and GEN42 probability rounding",
        ],
        [
            "test_gen41_nonfinite_total.csd",
            "expected failure: GEN41 probability total overflows",
            1,
        ],
        [
            "test_gen42_nonfinite_total.csd",
            "expected failure: GEN42 probability total overflows",
            1,
        ],
        ["test_ftload_binary_args_ownership.csd", "test binary ftload does not share args ownership"],
        ["test_getftargs_empty_after_ftload.csd", "test getftargs returns empty args after binary ftload"],
        ["test_fail_compilestr.csd", "testing clean compilestr fail"],
        [
            "test_realtime_compile_opcodes.csd",
            "test realtime init-thread compile opcodes",
        ],
        ["test_global_struct_var.csd", "testing global structure var"],
        ["test_setscorepos.csd", "testing setscorepos and rewindscore"],
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
        ["test_voice_init.csd", "voice initializes its SingWave helper"],
        ["test_grain3_overlap_regression.csd", "grain3 should not fail with false overlap error"],
        ["test_grain3_float_interpolation.csd", "grain3 float-path interpolation should stay positive"],
        ["test_instr_redefinition.csd", "allow instr redefinition"],
        ["test_instr0_labels.csd", "test labels in instr0 space"],
        ["test_string.csd", "test string assignment and printing"],
        [
            "test_strstrip_reallocation.csd",
            "test strstrip reallocation and termination",
        ],
        [
            "test_string_preprocessor_whitespace.csd",
            "preserve function-like whitespace inside strings",
        ],
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
        [
            "arrays/test_arrays_zero_dimension_fail.csd",
            "test expected failure with zero dimension in multi-dimensional array",
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
        ["test_for_loop_index_var_typed.csd", "for in loop with typed index var"],
        ["test_opcode_as_function.csd", "test expression"],
        ["test_fsig_udo.csd", "UDO with f-sig arg"],
        ["test_karrays_udo.csd", "UDO with k[] arg"],
        ["test_arrays_addition.csd", "test array arithmetic (i.e. k[] + k[]"],
        ["test_arrays_fns.csd", "test functions on arrays (i.e. tabgen)", 1],
        ["test_newstyle_passbycopy.csd", "test newstyle udo with setksmps"],
        ["test_polymorphic_udo.csd", "test polymorphic udo"],
        ["test_udo_a_array.csd", "test udo with a-array"],
        ["test_udo_2d_array.csd", "test udo with 2d-array"],
        ["test_udo_string_array_join.csd", "test udo with S[] arg returning S"],
        [
            "test_compilestr_udo_redefine_assert.csd",
            "test UDO redefinition via compilestr",
        ],
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
            "test prints does not crash when given a number arguments"
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
        [
            "test_fail_typed_ident_expression.csd",
            "expected failure: explicit type annotation used in expression",
            1,
        ],
        ["test_explicit_globals.csd", "test global declaration of explicit types"],
        [
            "test_mismatched_type_shadowing.csd",
            "test explicitly typed locals shadow globals with complete array metadata",
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
        ["test_midi_default.csd", "test midi default instr"],
        ["test_midi_tempo.csd", "test tempo reading from midifile"],
        ["test_midifiletempo_override.csd", "test midifiletempo before playback"],
        ["test_instr_type.csd", "test instr type and variables"],
        ["test_delete_instr.csd", "test creating and deleting instr"],
        ["test_create_instr.csd", "testing creating and scheduling instr"],
        ["test_instance_type.csd", "testing instance type"],
        ["test_play_opcode.csd", "testing play opcode"],
        ["test_splice_instance.csd", "test splicing instr order"],
        ["test_create_init_perf_delete.csd", "testing new instance opcodes"],
        ["test_complex_numbers.csd", "testing complex number operations"],
        ["test_rfft.csd", "testing real-to-complex and complex-to-real fft"],
        ["test_quadrature_osc.csd", "testing quadrature oscillator"],
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
        ["test_opcode_getp.csd", "tests numeric opcode-object output access"],
        [
            "test_opcode_getp_managed_fail.csd",
            "reject managed opcode-object output access",
            1,
        ],
        ["test_sa.csd", "test sample accurate mode"],
        [
            "test_local_ksmps_sample_accurate.csd",
            "test sample-accurate local ksmps offsets",
        ],
        [
            "test_local_ksmps_sample_accurate.csd",
            "test sample-accurate local ksmps offsets with PARCS",
            None,
            ["-nd", "--num-threads=2"],
        ],
        ["test_overload_selection.csd", "test wrong annotation case"],
        ["test_unschedule.csd", "test unscheduling events"],
        ["diskin_excess_channels.csd", "test sample accurate mode"],
        # Keep the CSD's null realtime backend: its RMS checks require paced
        # performance; suite-wide -n can outrun the asynchronous I/O worker.
        ["test_async_diskin.csd", "test diskin in rt async mode", None, ""],
        ["test_diskin2_nested_reuse.csd", "test nested diskin2 instance reuse"],
        ["test_midifile_ops.csd", "testing midifile opcodes"],
        ["test_midifile_malformed.csd", "reject malformed MIDI files"],
        ["test_midifile_loop.csd", "testing midifile tempo set, pos, loop"],
        ["test_midifile_seek_tempo.csd", "testing tempo restoration after midifilepos"],
        ["test_midifile_time.csd", "testing midifile time counting"],
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
        ["test_dbap.csd", "test dbap and dbapgains opcodes"],
        ["test_generic_chan.csd", "testing generic bus channel"],
        ["test_generic_chan_no_match.csd", "testing type mismatch for bus channel", 1],
        ["test_bus_channels.csd", "testing bus channels"],
        [
            "test_commandline_args.csd",
            "command line application arguments after -- are available through argv",
            0,
            "-nd",
            '-- concert.orc "first violin" --logfile=ignored ""',
        ],
        [
            "test_csoptions_args.csd",
            "CsOptions application arguments after -- are available through argv",
            0,
            "-nd",
        ],
        [
            "test_commandline_overrides_csoptions_args.csd",
            "command line application arguments override CsOptions arguments",
            0,
            "-nd",
            '-- concert.orc "first violin" --logfile=ignored ""',
        ["test_stm.csd", "testing stm opcodes"],
        ["test_stm_sequential_trace.csd", "testing sequential stm state trace"],
        ["test_stm_event.csd", "testing stm transition event ring"],
        [
            "test_stm_instances.csd",
            "testing shared definition, independent stm runners and instance overload",
        ],
        ["test_stm_lockfree.csd", "testing one stm writer with lock-free multicore observers"],
        [
            "test_stm_parallel_runners.csd",
            "testing independent stm writers on parallel multicore runners",
        ],
        ["test_stm_long_names.csd", "testing stm string outputs with long node names"],
        [
            "test_stm_writer_conflict.csd",
            "expected failure: stm rejects overlapping writer instruments",
            1,
            {
                "return_code": 1,
                "output_contains": "[stm] runner already has another active writer",
                "output_excludes": [
                    "Segmentation fault",
                    "AddressSanitizer",
                    "UndefinedBehaviorSanitizer",
                    "Abort trap",
                ],
            },
        ],
        ["test_stm_reinit_reset.csd", "testing stm reinit cleanup and reset cycle"],
        [
            "test_stm_lifetime.csd",
            "stm runner pins active observers and rejects stale handles",
            1,
            {
                "return_code": 1,
                "output_contains": [
                    "[lifetime] retained observer remained safe after owner ended",
                    "INIT ERROR in instr 30 (opcode stmonenter)",
                    "[stm] on enter/exit: invalid runner",
                ],
                "output_excludes": [
                    "Segmentation fault",
                    "AddressSanitizer",
                    "UndefinedBehaviorSanitizer",
                    "Abort trap",
                ],
            },
        ],
        ["test_stm_invalid_id.csd", "expected failure: stm rejects fractional node id", 1],
        [
            "test_stm_unknown_node.csd",
            "expected failure: stmnext rejects an unknown node name",
            1,
            {
                "return_code": 1,
                "output_contains": "[stm] stmnext: node 'Zeta' not found",
                "output_excludes": [
                    "Segmentation fault",
                    "AddressSanitizer",
                    "UndefinedBehaviorSanitizer",
                    "Abort trap",
                ],
            },
        ],
        [
            "test_stm_type_mismatch.csd",
            "expected failure: stm rejects a builder where a definition is required",
            1,
            {
                "return_code": 1,
                "output_contains": "[stm] stminstance: invalid definition",
            },
        ],
        ["signalflowgraphtest.csd", "test signal-flow graph opcodes"],
        [
            "test_signalflowgraph_lifetime.csd",
            "test signal-flow graph cleanup and ftgenonce",
        ],
    ]

    parcsTests = [
         [ "test_parcs_instance_overlap.csd",
                "PARCS does not enter one instrument instance twice"], 
          ["test_parcs_perf_error.csd", "PARCS exits on perf error",1]
              ]
    
    if not csoundExecutable.lower().endswith((".wasm", ".cwasm")):
        tests += parcsTests           



        tests += [[ "test_parcs_local_ksmps_kcounter.csd",
                   "PARCS kcounter test with local ksmps"]]

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
        ["arrays/test_empty_array_init.csd", "test zero-length array initialization"],
        [
            "arrays/test_struct_array_reshape_copy.csd",
            "reshaping a struct-array copy preserves its source",
        ],
        [
            "arrays/test_struct_array_k_write_after_copy.csd",
            "k-rate writes use prepared struct-array copy storage",
        ],
        [
            "arrays/test_opcode_array_managed_fail.csd",
            "reject managed scalar transfers through Opcode arrays",
            1,
        ],
        [
            "arrays/test_chncleararray_managed_fail.csd",
            "reject byte-wise clearing of managed array channels",
            1,
        ],
        [
            "arrays/test_chngeta_local_ksmps.csd",
            "read audio channel arrays with a local ksmps",
        ],
        ["test_local_ksmps_out.csd", "test output with local ksmps"],
        ["test_local_ksmps_in.csd", "test input with local ksmps"],
        ["complex_array_test.csd", "testing complex array ops"],
        ["test_array_channels.csd", "testing bus channels holding arrays"],
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
        [
            "structs/test_struct_array_direct_member_access.csd",
            "test direct member access on struct array elements",
        ],
        [
            "structs/test_struct_array_nested_member_access.csd",
            "test nested member access on struct array elements",
        ],
        [
            "structs/test_struct_array_k_index_member_access.csd",
            "test k-rate index member access on struct array elements",
        ],
        [
            "structs/test_struct_array_member_copy_fail.csd",
            "copying struct-array member out of a struct",
        ],
        [
            "structs/test_struct_array_member_with_scalar.csd",
            "copying a struct-array member containing scalar fields",
        ],
        [
            "structs/test_struct_member_array_struct_set.csd",
            "assigning into a struct-array member inside a struct",
        ],
        [
            "structs/test_recursive_struct_member_array_direct_index.csd",
            "direct indexing of recursive struct-array members",
        ],
        [
            "structs/test_struct_array_nested_condition_arg.csd",
            "nested struct-array member reads inside boolean expression args",
        ],
        [
            "structs/test_struct_init_partial_member_fails.csd",
            "fail when struct init provides only some members",
            "fail",
        ],
        [
            "structs/test_string_array_direct_member_index.csd",
            "direct indexing of a string-array struct member",
        ],
        [
            "structs/test_struct_arate_members.csd",
            "a-rate struct member write/read with ksmps above parse-time default",
        ],
        [
            "structs/test_struct_arate_members_negative.csd",
            "fail when a-rate struct members are initialized with constants",
            "fail",
        ],
        ["test_exitnowk.csd", "perf-time exitnow opcode"],
        ["test_udt_channel.csd", "testing user-defined type channel"],
        ["test_udt_chan_no_match.csd", "testing unmatched udt channel", 1],
    ]

    udoTests = [
        ["udo/fail_no_xin.csd", "fail due to no xin", 1],
        ["udo/fail_no_xout.csd", "fail due to no xout", 1],
        ["udo/fail_invalid_xin.csd", "fail due to invalid xin", 1],
        ["udo/fail_invalid_xout.csd", "fail due to invalid xout", 1],
        ["udo/test_udo_const_inargs.csd", "correct polymorphic UDO entry found"],
        ["udo/test_udo_xout_const.csd", "Constants as xout inputs work"],
        ["udo/pass_by_ref.csd", "Pass-by-ref works with new-style UDOs"],
        [
            "udo/test_deep_udo_deactivation.csd",
            "deep UDO chains deactivate without exhausting the C stack",
            None,
            runArgs,
            "",
            256,
        ],
        ["udo/test_args_in.csd", "Pass-by-ref connects args correctly."],
        ["udo/test_K_type.csd", "K-type arguments work with pass-by-ref"],
        [
            "udo/test_init_only_udo_frame_lifetime.csd",
            "Init-only UDO frames release structured temporary values",
        ],
        [
            "udo/test_init_only_udo_nested_cleanup.csd",
            "Retained nested UDO cleanup survives overlapping frame reuse",
        ],
        [
            "udo/test_init_only_udo_reserved_refs.csd",
            "Recycled init-only UDOs restore reserved instance references",
        ],
        [
            "udo/test_udo_init_only_conditional_perf_chain.csd",
            "init-only UDOs in conditionals do not install perf chains",
        ],
        ["udo/crashing_test.csd", "test for UDO crashing", 1],
        ["udo/test_udo_array_set.csd", "test UDO array setting"],
        [
            "test_udo_optional_after_instr.csd",
            "test new-style UDO optional args defined after instr",
        ],
        ["test_udo_recursion.csd", "test for UDO recursion depth exception", 1]
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

    mkirTests = [
      ["test_deconv.csd", "test deconv opcode"],
      ["test_ideconv.csd", "test init-time deconv opcode"],
      ["test_gen_deconv.csd", "test deconv gen"]
    ]

    tests += arrayTests
    tests += structTests
    tests += udoTests
    tests += maxallocTests
    tests += pfieldTests
    tests += mkirTests

    unknownExpectedFailures = expectedFailures.difference(test[0] for test in tests)
    if unknownExpectedFailures:
        logger.error(
            "Unknown expected-failure tests: %s",
            ", ".join(sorted(unknownExpectedFailures)),
        )
        return 1
    if expectedFailures:
        logger.info(
            "Runtime-specific expected failures: %s",
            ", ".join(sorted(expectedFailures)),
        )

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
    with runtime_working_directory() as working_directory:
        results = run_tests_parallel(
            tests,
            runArgs,
            max_workers,
            collect_result,
            working_directory,
        )
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
            elif arg.startswith("--runtime-executable="):
                runtimeExecutable = arg[21:]
            elif arg.startswith("--runtime-environment="):
                runtimeExecutable = arg[22:]
                logger.warning(
                    "--runtime-environment is deprecated and accepts only an "
                    "executable path; it does not parse arguments. Use "
                    "--runtime-executable with repeatable --runtime-arg options"
                )
            elif arg.startswith("--runtime-arg="):
                runtimeArguments.append(arg[14:])
            elif arg.startswith("--expected-failure="):
                expectedFailures.add(arg[19:])
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
