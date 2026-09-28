# Building Csound for ESP32-P4 and ESP32-S3

These builds produce a static Csound library and headers for your firmware.
The ESP32-P4 uses 32-bit RISC-V. The ESP32-S3 uses Xtensa. Each needs its own
compiler and build directory.

## Tools

Install CMake, Ninja, Bison 3.8 or newer, Flex, and the Espressif GCC toolchain for your chip.
The build uses picolibc. CI uses Espressif GCC 15.2.0 from the
[esp-15.2.0_20251204 release](https://github.com/espressif/crosstool-NG/releases/tag/esp-15.2.0_20251204).
Choose the archive for your host computer.

| Chip | Toolchain archive prefix | C compiler |
| --- | --- | --- |
| ESP32-P4 | `riscv32-esp-elf` | `riscv32-esp-elf-gcc` |
| ESP32-S3 | `xtensa-esp-elf` | `xtensa-esp32s3-elf-gcc` |

Put the toolchain's `bin` directory on `PATH`. If you already use ESP-IDF,
activate its environment with `. "$IDF_PATH/export.sh"`. The selected
toolchain must include `picolibc.specs`.

On macOS, put Homebrew's Bison ahead of the old system copy on `PATH`, or pass
`-DBISON_EXECUTABLE="$(brew --prefix bison)/bin/bison"` when configuring.

## Build

Run these commands from the Csound source directory for ESP32-P4.

```sh
cmake -S . -B build-esp32p4 -G Ninja \
  -DCSOUND_ESP32_TARGET=esp32p4 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build-esp32p4/install"
ninja -C build-esp32p4
cmake --install build-esp32p4
```

For ESP32-S3, use a separate build directory.

```sh
cmake -S . -B build-esp32s3 -G Ninja \
  -DCSOUND_ESP32_TARGET=esp32s3 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build-esp32s3/install"
ninja -C build-esp32s3
cmake --install build-esp32s3
```

If the compiler is outside `PATH`, add
`-DCSOUND_ESP32_GCC=/full/path/to/riscv32-esp-elf-gcc` or
`-DCSOUND_ESP32_GCC=/full/path/to/xtensa-esp32s3-elf-gcc` to the configure command.
Use a fresh directory when changing chips or toolchains.

## Settings

`CSOUND_ESP32_TARGET` selects the toolchain and defaults for a bare metal build.
It enables `BARE_METAL` and sets `USE_DOUBLE=OFF` and `USE_FLOAT=ON`. Both
`cs_float` and `cs_double` then use `float`. It disables desktop audio drivers,
thread support, dynamic opcode plugins, and command-line tools.

The P4 build uses `-march=rv32imafc_zicsr_zifencei -mabi=ilp32f`. The S3 build
uses `-mlongcalls`. Both use picolibc, separate function and data sections,
`-fno-builtin`, and `-fno-exceptions`. C++ also uses `-fno-rtti`.
`NO_SERIAL_OPCODES` disables the desktop serial opcodes.
These settings follow the build files supplied by Aman Jagwani.

You can override the precision defaults with CMake options. To keep internal
double precision with float samples, add `-DUSE_FLOAT=OFF`. To use double
precision for both types, add `-DUSE_DOUBLE=ON -DUSE_FLOAT=OFF`.
`FORCE_SINGLE_PRECISION` does not select a Csound precision mode.
See [numeric types](../../doc/numeric-types.md) for the precision and ABI rules.

## Use the library

The default build installs `lib/libcsound.a` and `include/csound` below the
chosen prefix. Link the library into your firmware and use those installed
headers, including `float-version.h`. Your firmware must use matching
architecture, C library, and precision settings. It supplies the platform
startup code, memory, audio I/O, and any file support that your score needs.

CI builds and packages each chip separately. These jobs check compilation and
installation. They do not run audio on an ESP32 board.
