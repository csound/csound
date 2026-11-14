<CsTest>
description = "score statements longer than the reader's first text space are read in full"

[expect]
exit = 0
output = ["fields 12004 last 7", "fields 12004 last 8", "fields 1504 last 9"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  prints "fields %d last %d\n", pcount(), p(pcount())
endin
instr 2
endin
</CsInstruments>
<CsScore>
#define A #1 1 1 1 1 1 1 1 1 1#
#define B #$A $A $A $A $A $A $A $A $A $A#
#define C #$B $B $B $B $B $B $B $B $B $B#
#define Z #0000000000#
#define Y #$Z$Z$Z$Z$Z$Z$Z$Z$Z$Z#
; 12000 p-fields of one character, about 24 KB of text
i 1 0 0 $C $C $C $C $C $C $C $C $C $C $C $C 7
; the same, as an s statement's line
s $C $C $C $C $C $C $C $C $C $C $C $C
; carried p-fields
i 1 0 0 $C $C $C $C $C $C $C $C $C $C $C $C 8
i 1 0
s
; a b whose number grows the text space, then a long note
{ 200 N
i 2 0 0
}
b 0$Y$Y$Y$Y$Y$Y$Y$Y$Y$Y
i 1 0 0 $B $B $B $B $B $B $B $B $B $B $B $B $B $B $B 9
</CsScore>
</CsoundSynthesizer>
