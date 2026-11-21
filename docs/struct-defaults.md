# Struct field defaults (draft)

This draft lets an orchestra declare a default value for a struct field:

```csound
struct Event pitch:i, velocity:i = 100, label:S = "note"

instr 1
  event:Event init 60
  ; pitch = 60, velocity = 100, label = "note"
endin
```

The design still needs discussion before we move it forward.

## Values and constructors

Fields of type `i` or `k` accept finite number literals, with an optional `+`
or `-`. Fields of type `S` accept string literals, including the usual escapes
and the empty string. The draft does not accept expressions, variables,
function calls, or defaults for fields of array or struct types.

New structs use these defaults. Nested structs and new elements in arrays of
structs use their own field defaults too. Each value owns its string storage.

A positional `init` may omit trailing fields only when each omitted field has
a declared default. Supplied values override defaults, including zero and
empty strings. A copy constructor copies the source values without applying
defaults over them.

Calling `init` without arguments restores the declared defaults, including
those in nested structs. It leaves existing array elements and fields without
a declared default unchanged. The same rule applies when an instrument runs
again or enters a `reinit` section containing that `init`.

```csound
  event:Event init 60, 0, ""
  ; Explicit values: pitch = 60, velocity = 0, label = "".

  event init
  ; pitch stays 60; velocity returns to 100 and label to "note".
```

The plugin `RegisterStruct` API stays unchanged. This draft adds defaults to
orchestra declarations only.

## Points for discussion

- Whether defaults should stay limited to literals or later allow expressions.
- Whether positional constructors should allow omitted trailing fields.
- Whether no-argument `init` should restore defaults on an existing value.
- Whether JSON decoding should accept missing fields that have declared
  defaults, while still rejecting missing fields without defaults. See
  [JSON opcodes](json-opcodes.md) for the draft decoding rules.
