#!/usr/bin/env python3

# Csound Test Suite
# By Steven Yi <stevenyi at gmail dot com>

import logging
import locale
import os
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import time
from contextlib import contextmanager
from concurrent.futures import ThreadPoolExecutor, as_completed

from test_metadata import discover_tests

csoundExecutable = ""
sourceDirectory = os.path.dirname(os.path.abspath(__file__))
runtimeExecutable = None
runtimeArguments = []
selectedTests = set()
testProfile = None
listOnly = False
outputEncoding = locale.getpreferredencoding(False) or "utf-8"

# Parallel execution configuration
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


def is_normal_nonzero_exit(return_code):
    """Signals and Windows exception statuses never satisfy an expected error."""
    return 0 < return_code < 0x80000000 and return_code != 0x40000015


class TestResult:
    def __init__(self, test_index, test_data, return_code, cs_output,
                 execution_time, error=None, stderr_output="", stdout_output=""):
        self.test_index = test_index
        self.test_data = test_data
        self.return_code = return_code
        self.cs_output = cs_output
        self.execution_time = execution_time
        self.error = error
        self.filename = test_data.filename
        self.description = test_data.description
        self.stderr_output = stderr_output
        self.stdout_output = stdout_output

    @property
    def mismatches(self):
        missing = []
        for stream, outputs in (("stderr", (self.stderr_output,)),
                                ("stdout", (self.stdout_output,)),
                                ("output", (self.stderr_output, self.stdout_output))):
            for pattern in self.test_data.expect.get(stream, []):
                if not any(pattern in output for output in outputs):
                    missing.append(f"{stream} missing substring {pattern!r}")
            for pattern in self.test_data.expect.get(stream + "_regex", []):
                if not any(re.search(pattern, output) for output in outputs):
                    missing.append(f"{stream} missing regex {pattern!r}")
        return missing

    @property
    def passed(self):
        if self.error is not None:
            return False
        expected = self.test_data.expect["exit"]
        if expected == 0:
            status_matches = self.return_code == 0
        else:
            status_matches = is_normal_nonzero_exit(self.return_code) and (
                expected == "nonzero" or self.return_code == expected)
        return status_matches and not self.mismatches

    def get_formatted_output(self, counter, verbose=False):
        status = "[pass]" if self.passed else "[FAIL]"
        output = f"{status} Test {counter}: {self.description} ({self.filename})\n"
        output += (f"\tReturn Code: {self.return_code}\tExpected: "
                   f"{self.test_data.expect['exit']}\n")
        if self.error:
            output += f"\tError: {self.error}\n"
        for mismatch in self.mismatches:
            output += f"\t{mismatch}\n"
        if verbose:
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
    """Run against a writable fixture copy without changing the source tree."""
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
            metadata from the CSD's <CsTest> block]
        run_args: Arguments to pass to csound
    Returns:
        TestResult object containing execution results
    """
    filename = test_data.filename
    desc = test_data.description
    test_run_args = test_data.args
    application_args = test_data.application_args
    stack_limit_kb = test_data.stack_limit_kb

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

            stderr_file.flush()
            stderr_file.seek(0)
            stderr_output = decode_process_output(stderr_file.read())
            stdout_output = decode_process_output(result.stdout)
            cs_output = stderr_output
            if stdout_output:
                cs_output += "\n[STDOUT CAPTURED]:\n" + stdout_output

        execution_time = time.time() - start_time

        logger.debug(
            f"Completed test {test_index + 1}: {filename} in {execution_time:.2f}s"
        )

        return TestResult(test_index, test_data, return_code, cs_output, execution_time,
                          stderr_output=stderr_output, stdout_output=stdout_output)

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
        tests: List of discovered test cases
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
        tests: List of discovered test cases
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
    --source-dir=<path>            Test directory (default: this script's directory)
    --runtime-executable=<path>    Runtime executable placed before the Csound module
    --runtime-arg=<arg>            Runtime argument; repeat once per argument
    --test=<relative.csd>          Run one test; repeat to select several
    --list                         List discovered tests and skip reasons
    --profile=native|wasm          Select in-file platform expectations
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

    profile = testProfile or ("wasm" if csoundExecutable.lower().endswith(
        (".wasm", ".cwasm", ".js")) else "native")
    try:
        tests = discover_tests(sourceDirectory, profile)
        unknown = selectedTests.difference(test.filename for test in tests)
        if unknown:
            raise ValueError(f"unknown tests: {', '.join(sorted(unknown))}")
        if selectedTests:
            tests = [test for test in tests if test.filename in selectedTests]
    except ValueError as error:
        logger.error("%s", error)
        return 1
    for test in tests:
        if test.skip:
            print(f"[skip] {test.filename}: {test.skip}")
        elif listOnly:
            print(test.filename)
    if listOnly:
        return 0
    skipped = [test for test in tests if test.skip]
    tests = [test for test in tests if not test.skip]
    if not tests:
        logger.error("No runnable tests selected")
        return 1

    output = "".join(f"[skip] {test.filename}: {test.skip}\n" for test in skipped)

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
        nonlocal testPass, testFail, testFailMessages, output
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
        output += formatted_output
        output += "%s\n\n" % ("=" * 80)
        output += csOutput
        output += "\n\n"

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
    print("Tests Passed: %i\nTests Failed: %i\nTests Skipped: %i\n" %
          (testPass, testFail, len(skipped)))

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

    with open("results.txt", "w", encoding="utf-8") as report:
        report.write(output)

    return testFail


if __name__ == "__main__":
    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
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
                os.environ["OPCODE7DIR64"] = os.path.abspath(arg[15:])
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
            elif arg.startswith("--test="):
                selectedTests.add(arg[7:])
            elif arg == "--list":
                listOnly = True
            elif arg.startswith("--profile="):
                testProfile = arg[10:]
                if testProfile not in ("native", "wasm"):
                    sys.exit("Unknown profile: " + testProfile)
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

            else:
                sys.exit("Unknown argument: " + arg)

    results = runTest()
    sys.exit(1 if results else 0)
