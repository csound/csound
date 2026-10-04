# Plugin-defined `[]`

A plugin type can be read and written with `[]`, the way `+` works for it once
the plugin registers a `##add` entry. Csound lowers `v[...]` to an opcode, and
the type takes part by registering entries for that opcode. Arrays of any type
(`T[]`) need none of this: Csound indexes them itself.

This note says what Csound guarantees and what each plugin decides. The tests
in `tests/c/plugin_index_test.cpp`, with the example plugin
`tests/c/fixtures/indexed_plugin.c`, check every rule below.

## Entries

An entry indexes a type when its first input is exactly that type, such as
`:IndexedVec;` (not `:IndexedVec;[]`).

| Opcode | Used for | Inputs | Output |
|---|---|---|---|
| `##array_get` | reading `v[...]` | type, indices | the element |
| `##array_set` | `v[...] = x`, and opcode outputs into `v[...]` | type, value, indices | none |
| `##array_init` | `v[...] init x` | type, value, indices | none |

```c
{"##array_get.IVi", sizeof(VEC_GET), 0, "i", ":IndexedVec;i",  vec_get, NULL,         NULL},
{"##array_get.IVk", sizeof(VEC_GET), 0, "k", ":IndexedVec;k",  vec_get, vec_get_perf, NULL},
{"##array_set.IVi", sizeof(VEC_SET), 0, "",  ":IndexedVec;ii", vec_set, NULL,         NULL},
{"##array_set.IVk", sizeof(VEC_SET), 0, "",  ":IndexedVec;kk", NULL,    vec_set_perf, NULL},
{"##array_init.IVi", sizeof(VEC_SET), 0, "", ":IndexedVec;ik", vec_set, NULL,         NULL},
```

A type that has no `##array_get` entry cannot be read with `[]`. A type that
has neither `##array_set` nor `##array_init` cannot be written. A type without
`##array_init` rejects `init` on an element.

## What Csound guarantees

### When an access runs

Csound chooses an entry for the rate of the access. Your entry's init and
perf functions then decide when it runs, so register them to match:

- An `i` value slot or `i` output runs at init time only: init function, no
  perf function.
- A `k` value slot or `k` output runs at perf time: perf function, plus an
  init function for a read that is also needed at init.
- `##array_init` runs at init time only: init function, no perf function.

Csound does not check this. A `k` setter with an init function also writes
at init time, as `kvar =` must not.

Reads follow the rule for built-in arrays. A read follows changes, using the
entry that takes `k` indices, unless an init-only consumer reads it with `i`
indices. An `i` assignment is an init-only consumer. With `i` indices, the
element is typed by the init-time read when Csound chooses the statement's
overload: an opcode with both `i` and `k` versions takes the `i` one.

```csound
v:IndexedVec = indexed_vec(4)   ; holds 0, 10, 20, 30
kc init 0
kc += 1
v[0] = kc                       ; k value: perf time, every block
kvalue = v[0]                   ; follows: 1, 2, 3, ... in blocks 1, 2, 3, ...
ix = v[2]                       ; init-only: 20
```

Because of that typing, `chnset v[0], "c"` uses the `i` version of `chnset`
and sends the init-time value once. To follow changes, read into a `k`
variable first (`kvalue = v[0]`) or use a `k` index.

An assignment `v[...] = x` is k-rate when `x` or any index is k-rate.
Otherwise it runs once, at init time, like `ivar = x`. `init` always writes
once, at init time.

```csound
kidx init 0
v[kidx] = 7                     ; every block, at the index of that block
v[1] = 5                        ; once, at init time
kj init 2
v[kj] init 7                    ; once, at init time, at v[2]
```

When an opcode writes an element (`v[2] indexed_vec_len v`), the element has
the type it has when read: the init-time read with `i` indices, or the
perf-time read with any `k` index.

### Which entry an access uses

Registration order never matters. Csound considers the entries of the type
that take the arguments:

1. A writer whose value slot is exactly the value's type ranks above one that
   takes an `i` value in a `k` slot. Constants and p-fields are `i` values.
2. For a perf-time access, an entry that takes the indices as `k` ranks first.
   For an init-time access, an entry that takes only `i` indices ranks first.

If no entry takes the arguments, compilation fails with
`no ##array_set entry of type IndexedVec takes arg types IndexedVec, S, i`.
If two entries share the best rank, compilation fails with
`ambiguous ##array_get entries of type AmbiguousVec: 2 take arg types ...`.
Csound reports the ambiguity only for an access that meets it, so other
accesses to the same type still compile.

### How an access is evaluated

- `v[a][b]` is one access with two indices. Csound passes both to one entry
  and never indexes the result again.
- Each index expression runs once per access: `v[f()] = v[f()] + 1` calls
  `f` twice.
- `[]` works on variables, UDO arguments and the elements an opcode writes.
  `record.field[0]` and `f()[0]` are not supported. They are compile errors.
- Errors from your init and perf functions are reported with the statement's
  line.

## What the plugin decides

- **What the indices mean.** For `IndexedVec`, one index is an element and
  two are the slice `[from, to)`. A plugin could read two as a row and a
  column, or reject them by registering no such entry.
- **Copy or view.** A read that returns a container decides whether the
  result shares storage with `v`. The example's slice is a copy, so writing
  to it leaves `v` as it was.
- **Index policy.** Fractional, negative and out-of-range indices are the
  plugin's to handle. The example truncates toward zero (`v[1.7]` is `v[1]`)
  and fails with an init or perf error for a negative, NaN or out-of-range
  index. Check the sign before converting: a negative `cs_float` cast to
  `size_t` is undefined behaviour.
- **Which rates exist.** A type with only an `i` read cannot be read at perf
  time. A perf-time access then uses that `i` read, so `kv = p[1]` holds the
  value `p[1]` had at init time.
