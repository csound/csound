# Developer documentation

- [Csound 7 release notes and feature articles](../Release_Notes/Version_7.00.md)
- [Building Csound](../BUILD.md)
- [Submitting opcodes](Submitting_opcodes.md)
- [Csound 6 to 7 API migration](API_Migration_Guide_Csound_6_to_7.md)
- [Numeric types](numeric-types.md)
- [Opcode maintenance policy](opcode-deprecation.md)
- [Deprecated opcodes](deprecated-opcodes.md)
- [Instance variables](instance-variables.md)
- [JSON opcodes](json-opcodes.md)
- [Memory playback](memplay.md)

The `examples/` folder holds sample `.csoundrc` files for Unix and Windows.
`csound_system_documentation/` holds the system documentation sources.
Older Windows build notes remain in `How_to_Build_Csound_on_Windows.doc`.
The old change log lives in [`Release_Notes/ChangeLog`](../Release_Notes/ChangeLog).

## Doxygen API documentation

Doxygen configuration and support files live in `doxygen/`.
From the repository root, with Doxygen and Graphviz installed:

```sh
cmake -S . -B build -G Ninja -DBUILD_DOCS=ON
ninja -C build doc
```

CMake uses `doxygen/config.doxygen` as a template. Generated files go to
`build/docs/doxygen/`; the HTML entry point is `build/docs/doxygen/html/index.html`.
The standalone configuration also works from the repository root:

```sh
mkdir -p build/docs/doxygen
doxygen docs/doxygen/Doxyfile
```

The older `doxygen/csound.tex` and its LaTeX style remain for reference.
They depend on manual sources that this repository does not include.
