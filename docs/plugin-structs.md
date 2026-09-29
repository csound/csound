# Plugin-defined structs

Plugins can declare global UDTs for their opcode inputs and outputs. Include
`csdl.h`, which includes the public `csound_structs.h` interface. Users can then
use the type without a matching `struct` declaration in their orchestra.

## Declare a type

Register types in `csoundModuleCreate`, before Csound compiles the orchestra.
Register opcodes in `csoundModuleInit` as usual.

```c
PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    const CSOUND_STRUCT_MEMBER fields[] = {
        {"inote", "i"},
        {"ivelocity", "i"}
    };
    if (csound->RegisterStruct == NULL)
        return NOTOK;
    return csound->RegisterStruct(csound, "NoteTuplet", fields, 2)
        ? OK : NOTOK;
}
```

The type belongs to that Csound instance until reset. Csound copies the member
names and types, so the descriptor array may be local. Registration returns
`NULL` for an invalid definition or an existing name. Orchestra declarations
cannot replace a plugin's struct. Names use ordinary identifiers and share the
global type space. Choose a plugin-specific type name to avoid collisions.

Members use type names such as `i`, `k`, `S`, `OtherStruct` or `OtherStruct[]`.
Register referenced structs first. A struct may refer to itself through an
array, such as `Tree[]`; direct self-reference is not allowed. Names and member
names have a limit of 250 bytes. A struct has 1 to `VARGMAX` members.

## Use the type in an opcode

Use `:NoteTuplet;` in an opcode's input or output signature, or
`:NoteTuplet;[]` for an array. A struct argument points to a `CS_STRUCT_VAR`.
Its `members` array follows declaration order, and each member has a
`CS_VAR_MEM` type header and value.

Csound initializes output storage before calling the opcode. For the two
scalar `i` members above, the opcode can write:

```c
typedef struct {
    OPDS h;
    CS_STRUCT_VAR *out;
    cs_float *note;
    cs_float *velocity;
} NOTE;

static int32_t make_note(CSOUND *csound, NOTE *p)
{
    (void)csound;
    p->out->members[0]->value = *p->note;
    p->out->members[1]->value = *p->velocity;
    return OK;
}
```

Register this callback with output `:NoteTuplet;` and inputs `ii`. An opcode
accepting the struct also receives a `CS_STRUCT_VAR *` and can read its members.
Csound owns these arguments and their member blocks. Do not free them, replace
their pointers, or copy a whole struct with `memcpy`. Use the type's `copyValue`
callback for managed values such as strings, arrays and nested structs.

The [complete C plugin](../tests/c/fixtures/struct_plugin.c) declares the type,
returns a note, and accepts one as an input:

```csound
instr 1
  note:NoteTuplet = plugin_note(60, 100)
  pitch:i = plugin_note_pitch(note)
  prints "note=%g velocity=%g\n", pitch, note.ivelocity
endin
```

Registered types also support normal orchestra `init` constructors, member
access, copies and arrays. Existing `jsonmarshal` and `jsonunmarshal` opcodes
work with them like any other UDT.
