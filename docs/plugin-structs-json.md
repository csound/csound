# Plugin structs and JSON

Plugins can register global orchestra structs and use Csound's JSON conversion
code through the `CSOUND` function table. Include `csdl.h`; it includes the public
`csound_structs.h` and `csound_json.h` interfaces. No yyjson headers or separate
JSON library are needed.

## Register a struct

Register types in `csoundModuleCreate`, before Csound compiles the orchestra.
Register opcodes in `csoundModuleInit` as usual.

```c
PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    const CSOUND_STRUCT_MEMBER members[] = {
        {"inote", "i"},
        {"ivelocity", "i"}
    };
    if (csound->RegisterStruct == NULL || csound->GetJsonAPI == NULL ||
        csound->GetJsonAPI(CSOUND_JSON_API_VERSION) == NULL)
        return NOTOK;
    return csound->RegisterStruct(csound, "NoteTuplet", members, 2)
        ? OK : NOTOK;
}
```

The type belongs to that Csound instance until reset. Csound copies the member
names and types, so the descriptor array may be local. Registration returns
`NULL` for an invalid definition or an existing name. Orchestra declarations
cannot replace a plugin's struct. Names use ordinary identifiers; namespace
syntax is not part of this interface. Choose a plugin-specific type name when
publishing a plugin to avoid collisions.

Members use explicit type names such as `i`, `k`, `S`, `OtherStruct` or
`OtherStruct[]`. Register referenced structs first. A struct may refer to itself
through an array, such as `Tree[]`. Direct self-reference is not allowed. Names
and member names have a limit of 250 bytes; a struct has 1 to `VARGMAX` members.

Registered structs support the same member access, arrays, assignment and `init`
constructors as orchestra-defined structs. An opcode returning this struct uses
`:NoteTuplet;` as its output signature, or `:NoteTuplet;[]` for an array.

## Convert or edit JSON

Request the supported interface version:

```c
const CSOUND_JSON_API *json = csound->GetJsonAPI(CSOUND_JSON_API_VERSION);
```

For JSON whose keys already match the struct members, call `Unmarshal` or
`UnmarshalFile` with the opcode's initialized output argument. `Marshal` writes
an initialized input argument to a `STRINGDAT` output. Pass
`p->h.insdshead` as the instance. These calls share the validation, depth limits,
field-path errors and output ownership rules of `jsonmarshal` and
`jsonunmarshal`. Failed conversions preserve the previous output value.

For validation or edits before conversion:

1. Use `Parse` for a string with an explicit byte length, or `ParseFile` for a
   file resolved through Csound's file callbacks and search paths.
2. Inspect `Root`, `Kind`, `Member`, `Element`, `Size`, `Number`, `String` and
   `Boolean`. A missing value has kind `CSOUND_JSON_INVALID`, distinct from JSON
   `null`. Check the kind before reading a value. Array indexes are zero-based.
3. Use `Rename`, `SetNumber`, `SetString` or `Remove` to edit the document.
   Mutation calls return `OK` or `NOTOK`. Renaming to an existing key fails.
   Setters require the matching number/string kind. All key and string inputs
   are copied. Numeric reads use `double`; large integers may lose precision
   when a plugin reads or changes them through this interface.
4. Call `Decode` to populate an initialized UDT or typed array, or `Write` to
   produce a JSON string.
5. Call `Free` on the document on every exit path.

Documents own their values and strings. Do not free individual values, mix
values from different documents, or keep pointers after freeing a document.
Conversion copies data into Csound-owned output storage. Raw C structs without
Csound argument type headers are not valid conversion targets.

Read flags match `jsonunmarshal`: `1` allows comments, `2` allows trailing
commas, and `3` allows both. Parsing reports an error message and byte position
through `CSOUND_JSON_ERROR`. Conversion reports an initialization error.
Depth `0` selects the maximum of 256; lower conversion limits are allowed.
`Write` and `Marshal` take a `pretty` value of `0` or `1`.

These calls allocate memory and may read files. Use them during initialization,
not in real-time performance callbacks. The versioned interface keeps yyjson's
structures, allocator and symbols private to Csound.

## C plugin example

The loadable [test plugin](../tests/c/fixtures/json_struct_plugin.c) validates
MIDI note and velocity ranges, renames JSON keys, and returns `NoteTuplet[]`:

```csound
instr 1
  notes:NoteTuplet[] plugin_notes {{
    [{"note":60,"velocity":100},{"note":64,"velocity":80}]
  }}
  prints "note=%g velocity=%g\n", notes[0].inote, notes[0].ivelocity
  Sjson jsonmarshal notes
endin
```

No `struct NoteTuplet` declaration is needed in the orchestra. The file variant,
`plugin_notesfile`, takes a path to the same JSON data. The fixture also shows
how to call `Unmarshal`, `UnmarshalFile` and `Marshal` directly from an opcode.
The [C++ tests](../tests/c/plugin_struct_json_test.cpp) load the plugin and check
these calls through the public interface.
