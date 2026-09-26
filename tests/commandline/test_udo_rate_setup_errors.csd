<CsTest>
description = "UDO rate setup rejects opcode calls, conditional placement, and repeated settings"

[expect]
exit = "nonzero"
stderr = ["setksmps must precede opcode initialization in UDO FromOpcode", "oversample must precede opcode initialization in UDO ConditionalRate", "UDO TwoSettings may have only one rate setting", "undersample must precede opcode initialization in UDO PerfConditionalRate", "setksmps must precede opcode initialization in UDO LabelledRate"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

opcode GetBlock():i
  xout 2
endop

opcode FromOpcode():i
  ; Calculate opcode-based arguments in the caller instead.
  setksmps GetBlock()
  xout ksmps
endop

opcode ConditionalRate(iEnabled:i):i
  ; A rate setting must be at the start, even if this branch is skipped.
  if iEnabled == 1 then
    oversample 2
  endif
  xout sr
endop

opcode TwoSettings():i
  ; A no-op does not permit another rate setting in the same UDO.
  setksmps 0
  setksmps 2
  xout ksmps
endop

opcode PerfConditionalRate(kEnabled:k):i
  ; Performance-only conditions must not hide a rate setting either.
  if kEnabled == 1 then
    undersample 2
  endif
  xout sr
endop

opcode LabelledRate():i
  ; Rate setup must stay outside sections that reinit can enter directly.
SET_RATE:
  setksmps 2
  xout ksmps
endop

instr 1
  iBlock FromOpcode
endin

instr 2
  iRate ConditionalRate 0
endin

instr 3
  iBlock TwoSettings
endin

instr 4
  iRate PerfConditionalRate 0
endin

instr 5
  iBlock LabelledRate
endin
</CsInstruments>
<CsScore>
i 1 0 .25
i 2 0 .25
i 3 0 .25
i 4 0 .25
i 5 0 .25
</CsScore>
</CsoundSynthesizer>
