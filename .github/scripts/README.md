# CI build reuse and test groups

`setup-build` sets a CPU-based build limit. Unix jobs use Ninja. Windows jobs
build one MSBuild project at a time and let `/MP` use the runner's CPUs to
compile that project's source files. This keeps the combined compiler worker
count within the CPU limit. Docker build steps pass the limit and vcpkg cache
path into their containers. The legacy
Emscripten job keeps serial Make builds because its library targets write the
same archive. Its dependency cache remains enabled.

The vcpkg cache stores binary archives, not an installed dependency tree.
vcpkg still checks each package's build inputs before reusing it. Cache keys
include the runner image, architecture, job, vcpkg revision and build files.
Compiler caches also separate matrix variants, and ccache checks source,
headers, compiler contents and flags. Each compiler cache is limited to 256 MB.

`install-apt` shares downloaded packages between jobs with the same package
list, recommendation setting and runner image. APT still refreshes repository
metadata and checks package versions and hashes. It uses the runner's CMake
and Ninja when present. Recommendations stay enabled unless a caller opts out.
Native builds and their test groups use the same package list and cache key.
Downloads use Ubuntu's HTTPS archive on hosted x64 runners, with timeouts and
retries. Save package archives right after installation so later failures do
not discard them.

`fetch-archive` caches the pinned WASI, Wasmtime and ESP32 downloads by SHA-256.
It checks the checksum on every use, including cache hits, before extraction.

`download-vc-redist.ps1` retries failed or incomplete Visual C++ runtime downloads.
It requires a valid Microsoft signature before passing a file to the Windows
installer build, and stops after three failed attempts.

The macOS installer uses source revisions from `platform/osx/dependencies.env`.
Its cache key includes the SDK, compiler, build tools, both target architectures
and dependency build scripts. Save these libraries before packaging changes
their install names. Update a revision or build script to refresh that cache.

The macOS Homebrew and Linux sanitizer jobs each build once and publish a tar
archive, which keeps executable permissions and framework symlinks. Four test
jobs restore that build at the same path and use the same source commit.
`ci-test-shards.py` discovers both full suites, keeps CTest fixtures together,
and divides each suite between the runners. It checks each runner's actual
JUnit and CSD results against its assigned tests. The final jobs keep the
original check names and require every group to pass, with no missing or
repeated test results. Existing native skip rules still apply.

The matrix and the planner both use four groups. Change both together if the
runner capacity changes. Run `python3 tests/test_ci_test_shards.py` when editing
the planner or result checks.
