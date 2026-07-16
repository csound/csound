<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

struct InstrumentNote pitch:i
struct InstrumentShelf names:S[], notes:InstrumentNote[], length:i, hasParent:i, parent:InstrumentShelf[]

opcode MakeNote(pitch:i):InstrumentNote
  note:InstrumentNote init pitch
  xout note
endop

opcode MakeRootShelf():InstrumentShelf
  names:S[] init 1
  notes:InstrumentNote[] init 1
  parent:InstrumentShelf[] init 1
  shelf:InstrumentShelf init names, notes, 0, 0, parent
  xout shelf
endop

opcode MakeChildShelf(parentShelf:InstrumentShelf):InstrumentShelf
  names:S[] init 1
  notes:InstrumentNote[] init 1
  parent:InstrumentShelf[] init 1
  parent[0] = parentShelf
  shelf:InstrumentShelf init names, notes, 0, 1, parent
  xout shelf
endop

opcode FindInstrument(shelf:InstrumentShelf, name:S):i
  index:i = 0
  found:i = -1
  names:S[] = shelf.names

  while (index < shelf.length && found == -1) do
    if (strcmp(names[index], name) == 0) then
      found = index
    endif
    index += 1
  od

  xout found
endop

opcode StoreInstrument(shelf:InstrumentShelf, name:S, note:InstrumentNote):InstrumentShelf
  index = FindInstrument(shelf, name)

  if (index >= 0) then
    shelf.notes[index] = note
  else
    newLength:i = shelf.length + 1
    names:S[] init newLength
    notes:InstrumentNote[] init newLength

    copyIndex:i = 0
    while (copyIndex < shelf.length) do
      names[copyIndex] = shelf.names[copyIndex]
      notes[copyIndex] = shelf.notes[copyIndex]
      copyIndex += 1
    od

    names[shelf.length] = name
    notes[shelf.length] = note
    shelf.names = names
    shelf.notes = notes
    shelf.length = newLength
  endif

  xout shelf
endop

opcode LookupInstrument(shelf:InstrumentShelf, name:S):InstrumentNote
  index = FindInstrument(shelf, name)

  if (index >= 0) then
    note:InstrumentNote = shelf.notes[index]
  elseif (shelf.hasParent == 1) then
    note = LookupInstrument(shelf.parent[0], name)
  else
    note = MakeNote(-999)
  endif

  xout note
endop

instr 1
  winds:InstrumentShelf = MakeRootShelf()
  winds = StoreInstrument(winds, "flute", MakeNote(72))

  pit:InstrumentShelf = MakeChildShelf(winds)
  pit = StoreInstrument(pit, "timpani", MakeNote(43))

  flute:InstrumentNote = LookupInstrument(pit, "flute")
  timpani:InstrumentNote = LookupInstrument(pit, "timpani")
  missing:InstrumentNote = LookupInstrument(pit, "contrabassoon")

  prints "flute=%d timpani=%d missing=%d\n", flute.pitch, timpani.pitch, missing.pitch

  if (flute.pitch != 72 || timpani.pitch != 43 || missing.pitch != -999) then
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
