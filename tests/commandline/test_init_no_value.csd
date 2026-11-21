<CsTest>
description = "init with no value is an error, for b, B, i, k and S, not a crash"

[expect]
exit = "nonzero"
stderr_regex = [
  'instr 1 .*init needs a value',
  'instr 2 .*init needs a value',
  'instr 3 .*init needs a value',
  'instr 4 .*init needs a value',
  'instr 5 .*init needs a value',
  'instr 6 .*init needs a value',
  'instr 7 .*init needs a value',
  'instr 8 .*init needs a value',
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 32

instr 1
  first:b init
endin

instr 2
  first:b, second:b init
endin

instr 3
  first:B, second:B init
endin

instr 4
  first:b = init()
endin

instr 5
  first:i, second:i init
endin

instr 6
  first:k init
endin

instr 7
  first:S init
endin

; no outputs either, which `init 1` is refused for too
instr 8
  init
endin
</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
i3 0 0
i4 0 0
i5 0 0
i6 0 0
i7 0 0
i8 0 0
</CsScore>
</CsoundSynthesizer>
