# User-defined structs

[Csound 7 release notes](../Version_7.00.md)

A note can have a pitch, a start time, and a label. A Csound 7 struct
keeps those values in one typed variable. Its members can have different
types. You can pass the whole struct to a UDO or store it in an array.

## Declare members and access their values

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

With no constructor arguments, new numeric members start at zero and new
string members start empty. An array member starts without elements.
For member-based initialization, supply all members in declaration order.
Partial member lists are not valid.

## Pass a struct through a UDO

A UDO can accept and return a declared struct type. This example returns a
transposed copy of a note:

```csound
struct Note pitch:i, onset:i, label:S

opcode Transpose(note:Note, semitones:i):Note
  shifted:Note init note
  shifted.pitch += semitones
  xout shifted
endop

instr ChangeNote
  note:Note init 60, 0.125, "first"
  shifted:Note = Transpose(note, 7)
  print note.pitch, shifted.pitch
endin

schedule(ChangeNote, 0, 0)
```

The original pitch stays at `60`. The returned pitch is `67`.
The copy inside `Transpose` preserves the input. An assignment directly to
`note.pitch` inside this UDO would change the caller's member through the
input reference. The [UDO article](Language.md) explains these call rules.

## Combine structs and arrays

A struct can contain another struct or an array. An array of structs can
hold a sequence of notes:

```csound
struct Note pitch:i, onset:i, label:S
struct Phrase notes:Note[], name:S

instr ReadPhrase
  notes:Note[] init 2
  notes[0].pitch = 60
  notes[1].pitch = 67
  phrase:Phrase init notes, "pair"
  prints "%s: second pitch=%g\n", phrase.name, phrase.notes[1].pitch
endin

schedule(ReadPhrase, 0, 0)
```

`notes` contains two initialized structs. The `Phrase` constructor takes
that array and a name. Member access and array indices can appear together,
as in `phrase.notes[1].pitch`.

Initialize an array member before you use an index. You can pass an existing
array to the constructor, as above. Each member retains its declared type,
including its signal rate where applicable.

The [struct examples](../../tests/commandline/structs/) cover nested members,
arrays, and assignments. Plugins can also
[register struct types](../../docs/plugin-structs.md) for orchestra use.
