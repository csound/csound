# Orchestra modules

Csound orchestra modules share reusable UDOs, global variables, struct types and
named instruments between orchestra files. Each imported `.orc` file has its own
namespace and global storage. Imports preserve the original definitions and
storage, including the rate and element type of a variable.

This document describes the supported syntax, execution model and implementation.

## A complete example

Create `lib/voices.orc`:

```csound
gkGain init 0.1

opcode Tone(frequency:k):a
    aSignal oscili gkGain, frequency
    xout aSignal
endop

instr Voice
    aSignal = Tone(p4)
    outs aSignal, aSignal
endin
```

Create `main.csd` next to the `lib` directory:

```csound
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 2
0dbfs = 1

import "lib/voices.orc" as voices

instr Main
    aSignal = voices.Tone(440)
    outs aSignal, aSignal
    schedule voices.Voice, 0, 0.1, 660
endin
</CsInstruments>
<CsScore>
i "Main" 0 0.1
e
</CsScore>
</CsoundSynthesizer>
```

Run `csound main.csd`. The example compiles and performs without writing audio
because it uses `-n`. Select an audio output in `CsOptions` to hear it.

## Import syntax

Import statements appear at orchestra global scope. Paths are quoted strings.

| Statement | Names available in the importing file |
| --- | --- |
| `import "voices.orc"` | All exported names directly, plus qualifier `voices`. |
| `import "voices.orc" as synth` | All exported names directly, plus qualifier `synth`. |
| `from "voices.orc" import Tone, gkGain` | Only the requested names, directly. |
| `from "voices.orc" import Tone as MyTone` | The requested definition under its local name. |
| `from "voices.orc" import *` | All exported names directly. |

For a plain import, the default qualifier is the file's basename without `.orc`.
Use `as` when that basename is unsuitable as an identifier.

All module-level globals, UDOs, struct types and named instruments are exported.
Names imported by a module can also be re-exported. Local variables inside a UDO
or instrument belong to that definition. There is no explicit `export` declaration
or public/private modifier.

Plain imports deliberately provide both qualified and direct access. Prefer
selective imports and qualified calls when libraries export overlapping names.
Later imports can replace an earlier imported binding or qualifier in the same
file. Additional selective imports from the same source keep earlier bindings.
Use a selective alias when an imported name conflicts with an existing
identifier. Local variables can shadow global variables.

An alias belongs to the importing file. Reusing an alias in a different module
does not change the first file's bindings. A dependency's module aliases are used
inside that dependency; they are not automatically exported as new qualifiers.
Qualified calls use `alias.Opcode`, and a qualified variable may be followed by
struct members, such as `geometry.gOrigin.x`.

Missing files, missing requested items and circular imports are compilation errors.
A selective import does not make omitted definitions available by name.

## UDOs and overloads

Qualified UDOs support statement syntax and function-call syntax:

```csound
import "voices.orc" as synth

aSignal synth.Tone 440
aOther = synth.Tone(660)
```

Zero-argument calls use the same syntax, for example `settings.DefaultGain()`.
An imported UDO name retains its overload set. Selective renaming changes the
local name, while calls continue to use the original definitions and signatures.

A UDO resolves globals, types and imported names in its defining module.
Redefining a root UDO does not replace a same-named UDO in an imported module.

## Variables, arrays and shared state

Traditional globals use their normal prefixes, such as `giCount`, `gkGain` and
`gSLabel`. Explicitly typed globals use `@global`:

```csound
giCount init 0
gSLabel = "library"
giValues[] fillarray 10, 20, 30
```

An imported name refers to the source variable. Renaming an i-rate variable to a
name beginning with `gk` does not change its rate. Array dimensions, element types
and struct descriptors are retained with the binding.

```csound
import "settings.orc" as first
import "./settings.orc" as second
from "settings.orc" import giCount as counter

first.giCount = 7
; second.giCount and counter now read the same storage and value.
```

Each canonical file is one module instance within a Csound instance. Importing it
under several names shares state. Distinct files have independent state, even if
their exported identifiers are identical. `csoundReset` discards the module cache
and its state; importing a previously loaded path does not reload an edited file.

## Struct types

Module struct names have module scope. Two files can each declare `Point` with
different layouts. Import the types under distinct local names when both are
needed in one file.

For example, `geometry.orc`:

```csound
struct Point x:i, y:i

gOrigin@global:Point init 0, 0

opcode Translate(p:Point, dx:i, dy:i):Point
    result:Point init p.x + dx, p.y + dy
    xout result
endop
```

The importing instrument can use a renamed type, the shared global and the UDO:

```csound
from "geometry.orc" import Point as Position, gOrigin, Translate

instr 1
    position:Position = Translate(gOrigin, 10, 20)
    positions:Position[] init 2
    positions[0] = position
    prints "Position: %g, %g\n", positions[0].x, positions[0].y
endin
```

Type aliases work in annotations, struct members and typed UDO input/output
signatures, including arrays. A type omitted from a selective import is
unavailable by its source name. Use new-style typed UDO syntax for struct
parameters; old-style comma-separated struct type names are not supported.

An accepted root struct redefinition creates a new descriptor. Existing templates
and values keep the earlier descriptor and layout. Rejected edits do not replace
the active type. Public root type names retain the ordinary `:Point;` form;
compiler identities used to distinguish modules and edited layouts are private.

## Named instruments

Module instruments must be named:

```csound
from "lib/voices.orc" import Voice as ImportedVoice

instr Main
    schedule ImportedVoice, 0, 1, 440
    schedule "ImportedVoice", 1, 1, 660
endin
```

A qualified reference also works: `schedule voices.Voice, 0, 1, 440`, after a
plain import binding `voices`. String scheduling uses the unqualified imported
name or its selective alias, as shown above.

Imported instruments are registered automatically in the existing runtime
instrument table. Their numeric IDs are allocated by the engine; source modules
should not depend on a particular assigned number. Two modules can define
same-named instruments without sharing their templates or global storage.
Rebinding an imported name does not retire the source module's instrument.

No explicit registration opcode is required. Numbered instrument declarations
are rejected inside imported modules; the main orchestra can still use numbered
instruments and normal score events.

## File resolution and includes

The `.orc` extension is optional. Relative module paths are searched in this order:

1. The importing file's directory.
2. The main CSD's directory.
3. Directories from `INCDIR`, using Csound's existing search-path order.
4. The current working directory.
5. `./lib/`.
6. `./modules/`.

Csound's environment search-path parser visits later directory entries first.
For example:

```sh
csound --env:INCDIR=/shared/modules main.csd
```

A host uses the existing option interface:

```c
csoundSetOption(csound, "--env:INCDIR+=/shared/modules");
```

Set the paths before compiling imports. Absolute paths are accepted. The resolved
canonical path is the module cache key and the identity used for cycle detection.
Equivalent relative spellings therefore share one module instance.

A `#include` inside a module resolves relative to that module's directory, followed
by the preprocessor's normal include search. Diagnostics identify the module
source. An include is textual preprocessing; an import establishes a module
namespace, cache identity and initialization dependency.

## Compilation and initialization

Dependencies are parsed before the importer is verified. Their resolved symbols
are available for type checking, but their global code does not execute while
parsing. New dependency templates compile into the enclosing edit's engine state
and are prepared, validated and published together.

Initialization executes dependencies before their importers. A module's global
initializer runs once per Csound instance, including when several import paths
reach the same shared dependency. Initial module code runs during normal startup.
New modules introduced by live compilation initialize after publication, before
the importing edit's global code executes.

`--syntax-check-only` validates code without running either root or module global
code. System constants such as `sr`, `ksmps` and `nchnls` require numeric header
values; imported globals cannot supply those header values.

A parsed root edit has a private namespace. Deleting an uncompiled parse tree or
rejecting an edit leaves the active bindings intact. Accepted asynchronous edits
make compiler bindings available to subsequent requests; their UDO definitions
become visible to runtime lookup only at merge. Discarded edits do not execute
module initializers. Initialization errors use normal Csound error reporting;
external effects performed during execution are not transactional.

## Implementation and compatibility

`Engine/csound_module.c` owns canonical loading, import binding and private module
metadata. A `CS_MODULE` owns its variable declarations, overload sets, type
bindings, qualifiers and dependency information. Imported variables borrow the
original `CS_VARIABLE`; they are not inferred from an AST or copied into new
placeholder variables.

The installed layouts of `CS_VAR_POOL`, `TYPE_POOL` and `INSTRTXT` match develop.
Borrowed variable bindings and instrument scope/dependency records are stored
privately, keyed by their owning pool or template. Normal pool/template disposal
releases these records, and reset clears the entire module state.

The public type-pool lookup retains its context-free contract. It exposes root
and globally registered plugin types; module aliases belong to compiler scope.
Compiler-only lookup receives explicit engine context and resolves local
definitions and imports. Public plugin struct registration stays global when
called from a variable constructor during parsing, including legacy direct type
registration. Public type descriptors keep source names; private compiler signatures
retain module/version identities. The plugin opcode lookup interface accepts
public root signatures and exposes public signatures in the returned entry.

Named instrument templates retain their defining module through private metadata.
Internal registry names isolate module instruments while ordinary `InstrDef`
bindings and imported string aliases use the shared runtime registry.

The enclosing global template records new dependencies in initialization order.
`Engine/insert.c` executes their initializers through the existing instrument-zero
lifecycle, checking initialization separately from compilation. Module execution
does not require another runtime engine or a module-to-root storage repair pass.

Parsed trees, queued edits and running instances may retain namespace versions
and struct descriptors. Those versions remain valid until reset. Private metadata
must not be stored in new fields of installed public structs or recovered by
reading beyond an externally allocated public object.

## Regression coverage

Module CSDs live in `tests/commandline/imports/`; focused regressions are in its
`regressions/` subdirectory. Native host tests are in `tests/c/module_tests.cpp`.
`tests/c/fixtures/module_abi_layout.h` freezes the relevant develop layouts for
compile-time compatibility checks.

Coverage includes shared state, overloads, type aliases, omitted symbols, distinct
struct layouts, named instruments, relative paths, cycles, dependency ordering,
syntax-only validation, rejected edits, direct parse/compile ownership and queued
publication. Compatibility checks also exercise ordinary root type names, public
constructor lookup, caller-owned type pools, edited layouts and reset cleanup.

Build and run the project suites from the configured build directory:

```sh
make -j
make csdtests
OPCODE7DIR64="$PWD" ./tests/c/unittests --gtest_filter='ModuleSystemTests.*'
```

For binary compatibility validation, also compile a host or plugin against develop
headers and load that unchanged binary against the module build. Rebuilding all
consumers with the new headers alone does not test compatibility with old binaries.
