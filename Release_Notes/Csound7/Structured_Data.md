# Structs and JSON data

[Csound 7 release notes](../Version_7.00.md)

A note can have a pitch, a start time, and a label. A Csound 7 struct
keeps those values in one typed variable. You can pass the variable to a
UDO, put it in an array, or read it from JSON.

## Define the data in the orchestra

The `struct` declaration gives each member a name and type. Declare it in
the orchestra header, before its first use.

```csound
struct Note pitch:i, onset:i, label:S

instr ReadNote
  note:Note init 60, 0.125, "first"
  copy:Note init note
  copy.pitch = 67
  prints "%s: pitch=%g onset=%g\n", copy.label, copy.pitch, copy.onset
  print note.pitch
endin

schedule(ReadNote, 0, 0)
```

The member order sets the constructor argument order. A dot selects a member,
as in `copy.pitch`. This example changes the copy's pitch. The original
note keeps pitch `60`.

A struct can contain another struct or an array. An array of structs can
hold a sequence of notes. With no constructor arguments, new numeric members
start at zero and new string members start empty. An array member starts
without elements. Initialize it before you use an index.

For member-based initialization, supply all members in declaration order.
Partial member lists are not valid.

## Read and write JSON

The JSON opcodes use the struct declaration to determine the expected data.
Field names must match member names, including case.

```csound
struct Note pitch:i, onset:i, label:S

instr ReadJson
  text:S = {{ {"pitch":60,"onset":0.125,"label":"first"} }}
  note:Note jsonunmarshal text
  note.pitch = 67
  result:S jsonmarshal note, 1
  prints "%s\n", result
endin

schedule(ReadJson, 0, 0)
```

`jsonunmarshal` reads a string. `jsonunmarshalfile` reads a file.
`jsonmarshal` returns a JSON string. Its second argument selects indented
output when set to `1`.

These opcodes run at initialization and allocate memory. File input can
also block the engine. Load the data before real-time playback starts.

The root value must be a struct or a one-dimensional typed array.
Nested structs and arrays support data with several levels. Numeric,
Boolean, and string members retain their declared types. Audio signals and
live object handles cannot become JSON values.

Missing, unknown, or duplicate fields cause an error. A JSON string does
not become a number through automatic conversion. Strict JSON is the default.
Read flags can permit comments and trailing commas.

In a browser, file input uses the host's virtual filesystem. Put the file
there first, or pass JSON text to `jsonunmarshal`.

The [JSON guide](../../docs/json-opcodes.md) gives read flags, depth limits,
and numeric precision rules. Plugins can also
[register struct types](../../docs/plugin-structs.md) for orchestra use.
