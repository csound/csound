# CI build reuse and test groups

`setup-build` sets a CPU-based build limit. Unix jobs use Ninja; Windows
jobs also enable parallel compilation within MSBuild targets. Docker build
steps pass the limit and vcpkg cache path into their containers. The legacy
Emscripten job keeps serial Make builds because its library targets write the
same archive. Its dependency cache remains enabled.

The vcpkg cache stores binary archives, not an installed dependency tree.
vcpkg still checks each package's build inputs before reusing it. Cache keys
include the runner image, architecture, job, vcpkg revision and build files.
Compiler caches also separate matrix variants, and ccache checks source,
headers, compiler contents and flags. Each compiler cache is limited to 256 MB.

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
