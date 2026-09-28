# Instance variables for opcode authors

`CreateInstanceVariable` and `QueryInstanceVariable` provide named storage for
opcodes in the same instrument instance. Call them through `CSOUND`, passing
the opcode's `h.insdshead` as the owner. Each nested UDO and subinstrument has
its own scope. A query never searches its parent or another note.

Use a fixed name for each opcode family, such as `myplugin.delay.pairing`.
Different families can store different structures without adding fields to
`INSDS`. The engine copies the name and clears new storage to zero.

```c
PairingState *state = csound->QueryInstanceVariable(csound, owner, name);
if (state == NULL) {
  int32_t result = csound->CreateInstanceVariable(csound, owner, name,
                                                  sizeof(PairingState));
  if (result != CSOUND_SUCCESS)
    return result;
  state = csound->QueryInstanceVariable(csound, owner, name);
}
p->state = state;
```

Creation returns `CSOUND_ERROR` for a null or foreign owner, an empty or null
name, a duplicate name, or a size outside the range accepted by
`CreateGlobalVariable`. Allocation failure returns `CSOUND_MEMORY`.
Query returns `NULL` when the owner or name is invalid or the entry is absent.
A duplicate creation leaves the existing value unchanged.

## Lifetime

An entry lasts through ties, reinit and all opcode deinit callbacks for its
owner. Reinit does not clear entries, since opcodes outside the reinit block
may still use them. Each opcode family must maintain its own source links
when a source is reinitialized or deleted. The store does not track opcode
pointers or decide which source a reader should use.

After deactivation, the engine hides all entries before it can reuse the
instance. This also applies when a note fails during init. The next note
starts with no entries. Creating an entry again clears its contents, while
reusing its allocation if it is large enough. The engine frees these
allocations when it frees the `INSDS`.

The engine owns the returned memory. Do not free it. Use opcode deinit to
release any resources held inside it. A cached pointer expires when its
owner deactivates. An init-only UDO can deactivate as soon as its init work
ends, so do not pass its storage out as a longer-lived handle.

Keep `AUXCH` and `FDCH` descriptors in opcode memory. Their engine-owned
chains can outlive a note, while this store clears its values for the next
note. Store pointers to those resources here when needed.

For the UGEN API, the factory supplies the default scope. An explicit context
supplies a separate scope. Delete all UGENs using a scope before deleting the
context or factory. The store cannot follow a UGEN that moves to another
context, so an opcode that caches a pointer must keep the same context for
that pointer's lifetime.

## Performance and concurrency

Create may allocate memory. Query scans only this instance's small list.
Resolve the entry during init and cache its pointer for performance. The
store adds no processing callback or shared mutex. It does not make allocation
safe for a real-time audio callback.

The store has no locks. The caller must serialize access to the same owner,
including changes during reinit and reads through cached pointers. Different
instances have separate lists. Use fixed names so repeated notes can reuse
storage without keeping an ever-growing list of unused names.

## Compatibility and scope

The two function pointers sit before the global-variable API in `CSOUND`,
which shifts the later function-table offsets. The opaque pointer in `INSDS`
changes its size and p-field offsets. Plugins must be rebuilt with the new
headers.

This interface supplies ownership and storage. Opcode families still decide
their pairing order and when to remove their links. Explicit handles remain
the right choice for new opcodes that need to share data between instances.
