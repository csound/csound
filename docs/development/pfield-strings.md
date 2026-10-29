# P-field strings in Csound 7

`H/pfield.h` defines the engine's typed p-field storage. Each field holds a
number or a string index with an explicit kind. The string buffer belongs to
the set; offsets and its byte length bound lookups. Numeric NaNs and infinities
remain numbers in this storage.

Scheduling with string arguments and `subinstr` build typed sets. Legacy
score and host events enter through `csoundPFieldsImport`. For events with strings, the instance takes
its own copy before initialization, so the queue can release the event when
initialization ends. A string set includes p0 for indexing, but import does
not read p0: some legacy callers leave it unset.

## Private instance state

`H/insds_private.h` defines `struct insds_private`, which contains a `pfields`
member. The existing `INSDS.private_data` pointer owns this struct. Add future
state as named members here, with cleanup for each member. There is no fixed
number of spare slots and no need to change the public `INSDS` layout.

Deinit can still read the strings. Turnoff then releases the p-field set;
instance destruction also frees the private struct. Tied events replace the
set with the new event's values. Reinit keeps the current set. UDO string
lookup follows the parent instance chain, including nested UDOs. A
subinstrument owns its own set.

The field kinds record the incoming event, not every later write to a raw
p-field address. Csound 7 still allows opcodes to write those addresses and
allows string codes in numeric variables and arrays. Runtime legacy reads
therefore decode the current scalar value against the owner's string table;
they do not override a numeric write with the incoming field kind.

## Legacy adapter

`csoundPFieldStringCode` and `csoundPFieldStringIndex` contain the shared
16-bit string-index encoding. Score parsing, realtime text events, scheduling
and subinstruments use it. Export makes an owned `EVTBLK`; callers release it
with `csoundPFieldsFreeEvent`. Import and instance storage copy their input.
These operations use the caller's event or instance synchronization.

The public layouts of `INSDS`, `EVTBLK`, and `CS_VAR_MEM`, the inline
`IsStringCode` check, and the `GetArgString` function pointer remain unchanged.
Old plugins and existing orchestra code can still copy string codes through
numeric `pindex`, `passign`, arrays and UDO arguments.

Checked readers reject invalid indices before walking a legacy string list.
The old value-only reader returns an empty string for an invalid code because
its callers expect a C string. The checked internal readers return NULL.
`EVTBLK` has no buffer length: callers must still supply valid NUL-terminated
strings for its declared count. A non-NULL buffer with count zero retains the
old single-string convention; in-tree table generators now set the count.
The typed form has both an explicit count and byte extent.

A legacy event cannot distinguish numeric NaN from string ordinal zero when
it contains strings. The float `IsStringCode` check also accepts infinities.
Import cannot recover a caller's intent. Export rejects numeric values that
the old predicate would treat as strings when the set contains strings.
This restriction applies to the adapter, not typed storage.

## Csound 8

Complete removal needs a language and plugin transition: typed host events,
context-aware plugin reads, and rules for numeric copies and raw writes.
Score parsing, queues, table events, UDOs and consumers must migrate together
before the legacy encoding can go away. The [Csound 8 roadmap issue](https://github.com/csound/csound/issues/3505)
tracks that work.

This change keeps the compiler's NaN/Inf protections. Changing `qnan` and
`qinf` alone would not settle the other numeric NaN/Inf checks in the engine.
Revisit the vector-math work in [#3491](https://github.com/csound/csound/issues/3491)
separately, with defined special-value behavior.
