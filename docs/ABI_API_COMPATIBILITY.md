# API and ABI compatibility

Preserve supported host and plugin interfaces within a released major version,
including Csound 7.x. API compatibility keeps existing source code usable.
ABI compatibility keeps existing binaries working without a rebuild, on the
same platform and with matching [precision settings](numeric-types.md).

## Preserve public interfaces

- Keep existing public function names, exported symbols, parameter and return
  types, and calling conventions.
- Preserve public struct member types, order, alignment and size. Even adding
  a member at the end can break callers that allocate or embed the struct.
- Keep existing function-table entries at their current offsets. Preserve
  public enum values and flags that hosts or plugins pass across the interface.
- Respect documented ownership, lifetimes, units and return values. A change
  can break callers even when the binary layout stays the same.

## Extend the plugin function table through reserved slots

The public part of `CSOUND` in [csoundCore.h](../include/csoundCore.h) includes
reserved function-pointer slots. Add new entries at the start of the reserved
block and reduce the remaining reserve by the same number of slots. Keep the
table's total size and every existing entry's offset unchanged.

When the reserve runs out, make a compatibility decision before growing the
table. Plugins using a new entry must require a Csound version that provides it.

## Distinguish private implementation details

`CSOUND` is opaque to hosts, but its public function table is part of the plugin
ABI. The private data section below that table, guarded by
`__BUILDING_LIBCSOUND`, is hidden from hosts and plugins. Adding or rearranging
fields there does not by itself break the supported ABI.

Static helpers and [private headers in H/](../H/README.md) can also change.
Check what callers can see. Engine use alone does not make a type private.
For example, `INSDS` and `OPDS` are visible to plugins.

## Make compatibility decisions explicit

Before merging a break, explain the affected callers in the PR and obtain an
explicit maintainer decision on the target release and compatibility plan.
Prefer a new entry point, versioned type or compatibility wrapper when possible.

For public interface changes, compare exported symbols and relevant layouts
against the released version with the same build settings. Running newly
rebuilt hosts or plugins does not prove that existing binaries still work.
