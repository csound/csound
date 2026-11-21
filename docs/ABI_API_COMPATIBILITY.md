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

## Preserve the private CSOUND layout

`CSOUND` is opaque to hosts, but its public function table is part of the plugin
ABI. Its private data section also has ABI layout requirements. The
`__BUILDING_LIBCSOUND` guard controls visibility, not binary compatibility.
Rearranging private fields changes their offsets and breaks the ABI.

Within a released major version:

- Preserve every existing private field's type, size, alignment, order, and
  offset. Do not remove fields or insert new ones among them.
- Append new private fields only at the end of the entire private data
  section, after all existing fields. The end of a topic group is not the end
  of the struct.
- Keep the public function table's size fixed by using its reserved slots as
  described above. Growing it would shift the private section too.

Appending private fields to the engine-allocated `CSOUND` is permitted. This
does not relax the rule for public structs that callers allocate or embed.
Any change to an existing layout needs the compatibility decision described
below, even if all newly rebuilt code works.

## Check other implementation details

Static helpers and [private headers in H/](../H/README.md) can change when they
do not affect an ABI layout or a supported interface. Engine use alone does
not make a type private. For example, `INSDS` and `OPDS` are visible to plugins.

## Make compatibility decisions explicit

Before merging a break, explain the affected callers in the PR and obtain an
explicit maintainer decision on the target release and compatibility plan.
Prefer a new entry point, versioned type or compatibility wrapper when possible.

For interface or layout changes, compare exported symbols and relevant layouts
against the released version with the same build settings. Include existing
private `CSOUND` field offsets in that comparison. Running newly rebuilt hosts
or plugins does not prove that existing binaries still work.
