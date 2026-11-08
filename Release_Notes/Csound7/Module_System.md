# Orchestra modules

Orchestra files can be imported as reusable modules containing UDOs, global
variables, struct types and named instruments:

```csound
import "lib/voices.orc" as voices
from "geometry.orc" import Point as Position
from "settings.orc" import giGain
```

Imports preserve source storage and type identity. Each canonical module file
compiles and initializes once per Csound instance, with dependencies initialized
before importers. Live edits use the enclosing compiler's preparation and
publication lifecycle, and syntax-only validation does not execute module code.

Module paths use the existing `INCDIR` configuration. Public pool and instrument
layouts retain compatibility with develop; module metadata is kept privately.

See [Orchestra modules](../../docs/MODULE_SYSTEM.md) for the definitive syntax,
examples, namespace rules, instrument behavior, lifecycle, compatibility notes and
regression coverage.
