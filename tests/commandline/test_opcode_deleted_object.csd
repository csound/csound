<CsTest>
description = "expected failure: deleted opcode objects need create before reuse"

[expect]
exit = "nonzero"
stderr_regex = ["INIT ERROR in instr 1 .*opcode object not initialised", "INIT ERROR in instr 2 .*opcode object not initialised", "INIT ERROR in instr 3 .*opcode object not initialised"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

opcode Voice(f:k):a
  sig:a = oscili(0.1, f)
  xout sig
endop

; only the first note creates the objects, the second reuses the instance
instr 1
  if p4 == 0 then
    voice:Opcode = create(oscili)
  endif
  sig:a init 0
  sig init voice, 0.1, 440
  sig perf voice, 0.1, 440
  delete(voice)
endin

instr 2
  if p4 == 0 then
    voice:Opcode = create(opcoderef("Voice"))
  endif
  sig:a init 0
  sig init voice, 440
  sig perf voice, 440
  delete(voice)
endin

instr 3
  if p4 == 0 then
    voices:Opcode[] = create(opcoderef("Voice"), 2)
  endif
  freqs:i[] fillarray 220, 330
  sigs:a[] init 2
  sigs = run(voices, freqs)
  delete(voices)
endin
</CsInstruments>
<CsScore>
i 1 0 0.1 0
i 1 0.2 0.1 1
i 2 0 0.1 0
i 2 0.2 0.1 1
i 3 0 0.1 0
i 3 0.2 0.1 1
e
</CsScore>
</CsoundSynthesizer>
