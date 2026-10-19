<CsTest>
description = "delete on UDO opcode objects allows the instrument instance to be reused"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

#include "loops/libinstanced.orc"

opcode Voice(f:i):a
  sig:a = oscili(0.1, f)
  xout sig
endop

opcode Louder(f:i):a
  sig:a = oscili(0.3, f)
  xout sig
endop

instr 1
  ref:OpcodeDef = opcoderef("Voice")
  voices:Opcode[] = create(ref, 2)
  freqs:i[] fillarray 220, 330
  sigs:a[] init 2
  sigs = run(voices, freqs)
  assertSameSignal(sigs[0] + sigs[1], oscili(0.1, 220) + oscili(0.1, 330),
                   "object array")
  assertNotSilent(sigs[0], "object array")
  delete(voices)

  voice:Opcode = create(ref)
  sig:a = run(voice, 440)
  assertSameSignal(sig, oscili(0.1, 440), "single object")
  assertNotSilent(sig, "single object")
  delete(voice)
endin

; alternate definitions on a reused instance
instr 2
  name:S = p4 == 0 ? "Voice" : "Louder"
  ref:OpcodeDef = opcoderef(name)
  amp:i = p4 == 0 ? 0.1 : 0.3
  voices:Opcode[] = create(ref, 2)
  freqs:i[] fillarray 220, 330
  sigs:a[] init 2
  sigs = run(voices, freqs)
  assertSameSignal(sigs[0] + sigs[1], oscili(amp, 220) + oscili(amp, 330),
                   "changed definition")
  assertNotSilent(sigs[0], "changed definition")
  delete(voices)
endin
</CsInstruments>
<CsScore>
; later notes reuse the instance whose objects were deleted
i 1 0 0.1
i 1 0.2 0.1
i 1 0.4 0.1
; overlapping notes use separate instances
i 1 0.6 0.1
i 1 0.6 0.1
i 1 0.8 0.1
i 2 1 0.1 0
i 2 1.2 0.1 1
i 2 1.4 0.1 0
i 2 1.6 0.1 1
e
</CsScore>
</CsoundSynthesizer>
