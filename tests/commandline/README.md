# Command-line tests

The runner discovers every `.csd` below this directory, including subdirectories,
and runs them in path order. There is no Python test list or filename convention.
A new CSD runs automatically and expects exit status zero by default.

Put expectations in a JSON block inside a Csound comment, just after
`<CsInstruments>`. For example:

```csound
/* Csound-test
{
  "description": "reject managed opcode-array outputs",
  "expect": {
    "exit": "nonzero",
    "stderr": ["Opcode[] run does not support managed array output elements"]
  }
}
*/
```

`expect.exit` is an exact status from 0 to 255, or `"nonzero"`. An expected
failure must also declare a stderr diagnostic. Signals, Windows exception
statuses, timeouts, and runner errors always fail.

`stderr` and `stdout` are lists of required substrings. `stderr_regex` and
`stdout_regex` are lists of Python regular expressions, matched with `search`.
**All** listed checks must match on their named stream. JSON escapes apply:
write `"Opcode\\[\\]"` to match literal brackets. Match the stable diagnostic,
not a whole Csound log with version strings, times, or source line numbers.
Successful tests can declare output checks too.

Other optional fields:

- `description`: what the test checks; defaults to its relative path.
- `args`: Csound arguments before the CSD path; defaults to `["-nd"]`.
- `application_args`: arguments after the CSD path, as a list. Include `"--"`
  when passing application arguments. Spaces and empty arguments are preserved.
- `stack_limit_kb`: a positive POSIX stack limit; ignored on Windows.
- `skip`: a reason for a manual test or known blocker. Skips appear in the report.
- `profiles`: `native` or `wasm` overrides containing `expect` or `skip`.
  The runner selects `wasm` for `.wasm`, `.cwasm`, or `.js` executables;
  `--profile` overrides that choice. This replaces the old
  `--expected-failure=<file>` argument; keep platform exceptions in the CSD too.

Unknown keys, invalid expressions, and malformed metadata fail the run before
any Csound process starts. Csound itself ignores the comment, so files still
work when run directly. Tests run against a temporary writable copy of the
fixtures, with the copy's root as their working directory. Generated files do
not change the checkout. Tests running in parallel must use distinct output
filenames and ports.

From the project root:

```sh
ninja -C build csdtests
python3 tests/commandline/test.py --list
python3 tests/commandline/test.py --csound-executable=build/csound \
  --opcode7dir64=build --test=arrays/test_opcode_array_managed_output_fail.csd
python3 tests/commandline/test_harness.py
```

Repeat `--test` to select several paths. `--workers=1` runs sequentially;
`--timeout=60` changes the per-process timeout. See `--help` for runtime options.
The CMake targets also run the Python harness checks. Results go to `results.txt`
in the directory where the runner was invoked.
