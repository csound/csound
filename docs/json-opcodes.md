# JSON and user-defined types

`jsonunmarshal` reads a JSON string into a declared user-defined type (UDT)
or a one-dimensional typed array.
`jsonunmarshalfile` does the same from a file. `jsonmarshal` converts a value
back to a JSON string. UDT member names and types define the expected fields;
no separate schema is needed.

## Syntax

```csound
value:MyType jsonunmarshal Sjson [, iflags [, imaxdepth]]
value:MyType jsonunmarshalfile Spath [, iflags [, imaxdepth]]
values:MyType[] jsonunmarshal Sjson [, iflags [, imaxdepth]]
values:MyType[] jsonunmarshalfile Spath [, iflags [, imaxdepth]]
Sjson jsonmarshal value [, ipretty [, imaxdepth]]
```

All three opcodes run at initialization, not at k-rate. They allocate memory,
and the file opcode also performs file I/O. They are **not real-time safe**:
load and convert data before starting real-time playback, rather than in a
note that starts during playback.

## Example

```csound
struct Note pitch:i, onset:i, label:S

instr 1
  Sinput = {{ {"pitch":60,"onset":0.125,"label":"first"} }}
  note:Note jsonunmarshal Sinput
  prints "pitch=%g onset=%g label=%s\n", note.pitch, note.onset, note.label

  Scompact jsonmarshal note
  Sprettified jsonmarshal note, 1
  prints "%s\n", Sprettified
endin
```

An array can also be the root value. Using the `Note` type above:

```csound
instr 2
  notes:Note[] jsonunmarshal {{ [{"pitch":60,"onset":0.125,"label":"first"}] }}
  Sjson jsonmarshal notes
endin
```

The root must be an object mapped to a UDT or an array with a declared element
type. Root-level numbers, strings, booleans, and `null` are not supported.

The writer returns a string; it does not write a file. Use Csound's existing
string/file output opcodes to save that string.

## Read options

`iflags` defaults to `0`, which accepts strict JSON. Add these flag values to
enable each extension:

| Value | Input accepted |
| --- | --- |
| `0` | Strict JSON |
| `1` | `//` line comments and `/* ... */` block comments |
| `2` | A trailing comma in an object or array |
| `3` | Both comments and trailing commas |

These flags do not enable other JSON5 features, such as single-quoted strings,
unquoted keys, `NaN`, or infinity. Comments inside quoted strings remain text.
The writer always produces strict JSON; it does not preserve comments.

For the file call below, save the same JSON object as `settings.jsonc`.

```csound
struct Settings gain:i, name:S

instr 1
  Sinput = {{
    {
      "gain": 0.5, // linear gain
      "name": "soft",
    }
  }}
  settings:Settings jsonunmarshal Sinput, 3
  fromFile:Settings jsonunmarshalfile "settings.jsonc", 3
endin
```

`jsonunmarshalfile` opens files read-only and uses Csound's file search paths
`INCDIR`, `SSDIR`, and `SFDIR`. A string input never becomes a path by guessing:
choose `jsonunmarshalfile` when the argument names a file.

In a browser, place the file in the Csound host's virtual filesystem first.
The file opcode reads that filesystem; it does not fetch a URL or open an
arbitrary file on the user's computer. Use `jsonunmarshal` when the host already
has the JSON text.

## Types and validation

| Csound member type | JSON value |
| --- | --- |
| `i`, `k` | Finite number |
| `b`, `B` | Boolean `true` or `false` |
| `S` | UTF-8 string without NUL characters |
| A UDT | Object whose keys match its member names |
| A one-dimensional typed array | Array of values of its declared element type |

Nested UDTs and arrays of UDTs work the same way as scalar members. Reading
initializes `k` and `B` members; writing reads their values once during
initialization. JSON numbers do not substitute for booleans, or vice versa.
Audio signals and live opcode or instrument handles
are not supported.
Multidimensional Csound arrays are not supported; nest UDTs with array members
when the data needs several levels.

Field names are case-sensitive. JSON object fields may appear in any order,
but every declared member must appear exactly once. Missing, unknown, and
duplicate fields cause an error. `null`, wrong value types, and NUL characters
in keys or strings also cause errors; there is no implicit conversion from a
string to a number or from `null` to a default value.

An invalid input reports an initialization error without replacing the
destination with a partly decoded value. Syntax errors report a byte position;
value errors identify the field or array element when available.

Numbers use the build's `MYFLT` precision: 64-bit floating point in a double
build or 32-bit in a single-precision build. Reading rounds to that precision;
writing preserves the stored finite value without a user-selected rounding
step. This does not preserve the original decimal spelling or integers beyond
the precision of `MYFLT`.

## Output format and nesting limit

`ipretty` defaults to `0` for compact output. Set it to `1` for indented output.
Both forms contain the same data. Other values cause an error.

`imaxdepth` applies to both reading and writing. Its default, `0`, selects a
limit of 256 nested JSON containers. Set it to an integer from `1` to `256`
for a lower or explicit limit. The root object or array counts as one level; each
nested object or array adds one. Scalar values do not add a level.

For example, `{"notes":[{"pitch":60}]}` needs a limit of at least `3`:
the outer object, its array, and the note object. The limit also applies to
containers inside unknown fields. Inputs beyond the limit fail rather than
truncating the data.
