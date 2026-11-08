# Command-line tests

The runner discovers every `.csd` below this directory, including subdirectories,
and runs them in path order. There is no Python test list or filename convention.
A new CSD runs automatically and expects exit status zero by default.

Put TOML metadata inside a `<CsTest>` block at the top of the file, before
`<CsoundSynthesizer>`. For example:

```csound
<CsTest>
description = "reject managed opcode-array outputs"

[expect]
exit = "nonzero"
stderr = ["Opcode[] run does not support managed array output elements"]
</CsTest>
<CsoundSynthesizer>
...
</CsoundSynthesizer>
```

`expect.exit` is an exact status from 0 to 255, or `"nonzero"`. An expected
failure must also declare a stderr diagnostic. Signals, Windows exception
statuses, timeouts, and runner errors always fail.

`stderr` and `stdout` are lists of required substrings. `stderr_regex` and
`stdout_regex` are lists of Python regular expressions, matched with `search`.
**All** listed checks must match on their named stream. Use TOML literal strings
to avoid escaping regex backslashes: `'Opcode\[\]'` matches literal brackets.
Match the stable diagnostic, not a whole Csound log with version strings, times,
or source line numbers.
Successful tests can declare output checks too.

Use `output` or `output_regex` when the text may appear on either stream.
Csound's ordinary printed messages use stdout on Windows and stderr on Unix.
Each pattern must match within one stream; the runner does not join the streams
for matching. Expected failures still need an explicit stderr diagnostic.

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

Keep top-level fields such as `args` and `skip` before any TOML table headings.
For example, a WASM override uses its own table:

```toml
[profiles.wasm.expect]
exit = "nonzero"
stderr = ["unable to find opcode with name: OSCsend"]
```

Unknown keys, invalid expressions, and malformed metadata fail the run before
any Csound process starts. Csound ignores the metadata before
`<CsoundSynthesizer>`, so files still work when run directly. Tests run against a
temporary writable copy of the fixtures, with the copy's root as their working directory. Generated files do
not change the checkout. Tests running in parallel must use distinct output
filenames and ports.

The runner uses `tomllib` from Python 3.11 or later. On older Python versions,
install the `tomli` fallback with that interpreter:

```sh
python3 -m pip install -r tests/commandline/requirements.txt
```

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
