# Orchestra modules

An orchestra file can be imported as a reusable module. Module globals, UDOs,
struct definitions and named instruments are public by default.

```csound
import "lib/synths.orc" as synths
from "lib/settings.orc" import giGain as gain
from "lib/filters.orc" import *

instr Main
  aSignal = synths.Oscillator(440, gain)
  schedule synths.Voice, 0, 1
endin
```

`import "library.orc"` makes its exported names available directly and binds
`library` as a qualifier. `import ... as alias` uses the explicit qualifier;
it also keeps direct access to the exports for compatibility with the original
module syntax. Prefer selective imports or qualified names when exports overlap.
An alias belongs to the importing file. Reusing it in another module does not
change the first file's bindings. Reusing it in the same file replaces that
qualifier. A selective import can rename globals, UDOs, structs and instruments; additional
imports from the same file keep the earlier bindings. Requested names that do
not exist produce a compilation error.

Qualified UDOs support both statement and function-call syntax, including zero
arguments and overloaded signatures:

```csound
iValue synths.ReadValue 1
iValue = synths.ReadValue(1)
iDefault = synths.DefaultValue()
```

Imports retain the original variable and its type, including arrays and structs.
Assignments through an imported name or qualifier update the same storage.
Struct members can follow the qualifier, for example `settings.gPosition.x`.
Struct type names are scoped to their defining module. Different modules can
use the same struct name with different layouts. Selective type aliases work
in annotations, nested members and typed UDO signatures, including arrays:

```csound
from "geometry.orc" import Point as Position
position:Position init 10, 20
```

A selective import does not expose omitted struct names. Existing values keep
their original type layout when a later edit redefines a root struct.
Renaming a variable does not change its rate or type; renaming an i-rate global
to a name beginning with `gk` does not make it k-rate.

Named instruments retain their defining module's globals and can share a source
name with instruments in other modules. Imported instrument references work with
normal scheduling opcodes. Unqualified imported instrument names, including
renamed imports, are also available to string scheduling. Numbered instruments
are rejected inside modules.

## File resolution and initialization

Relative imports are resolved from the importing module's directory, then from
the main CSD directory, registered module search paths in registration order,
the working directory, `./lib/`, and `./modules/`. The `.orc` suffix is optional.
Absolute paths are accepted. Resolved canonical paths identify modules in the
cache and in circular-import detection, so `library.orc` and `./library.orc`
share one instance.

A module compiles once per Csound instance and initializes once, after its
dependencies. Shared dependencies in a diamond-shaped import graph do not run
twice. Initialization runs with normal orchestra global code, at startup or
after a successful live compilation. `--syntax-check-only` does not execute it.
New live imports use the same compiler state, preparation and publication as
the enclosing edit. Rejected imports do not replace existing aliases or UDO
bindings and can be corrected and retried. Parsing an edit does not publish
its bindings; discarding a parsed tree leaves the active namespace intact.
Asynchronous UDO definitions enter the runtime registry only when their edit
is merged. Relative `#include` paths inside a module use that module's directory,
and diagnostics identify the module source.

System constants such as `sr`, `ksmps` and `nchnls` still require numeric values
in the orchestra header. They cannot be assigned from a module global.

## Implementation

`CS_MODULE` owns its definitions, aliases and import metadata. `CS_VAR_POOL`
keeps owned declarations in `table`/`head` and borrowed imported variables in
`bindings`; freeing bindings never frees the source variable. UDO lookup retains
all signatures and includes the defining module in redefinition lookup.
Each parsed root edit has its own namespace and variable pool. Accepted queued
edits expose compiler bindings to subsequent edits, while runtime publication
uses a separate generation. Namespace versions and type descriptors remain
valid until engine reset, since parsed trees and running instances can retain
them. User-facing type names remain unchanged in diagnostics and `typeof`.
Compiled instrument and UDO templates retain that module as well. Runtime
instrument numbers remain global; module names are compiler scopes rather than
separate runtime engines.

New dependency templates are compiled into the enclosing `ENGINE_STATE` and
prepared together. A global template records its newly compiled dependencies
in dependency order. The existing global initializer executes them once before
the importing file's global code. Source AST scans, inferred placeholder types,
root-specific storage repairs and module-to-root self-merges are unnecessary.

Regression coverage lives in `tests/commandline/imports/regressions/` and
`tests/c/module_tests.cpp`.
