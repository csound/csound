<CsTest>
description = "taninv2 reports mismatched array shapes after reinitializing an operand"

[expect]
exit = "nonzero"
stderr = ["taninv2: array shapes do not match; reinitialise the opcode"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1

instr ControlShapeChange
  kCycle timeinstk
  kColumns init 3
  if kCycle == 2 then
    kColumns = 2
    reinit RESIZE
  endif
RESIZE:
  ; Only the second operand changes shape. taninv2 must report this.
  kX[][] init 3, i(kColumns)
  rireturn
  kY[][] init 3, 3
  kAngles[][] = taninv2(kY, kX)
endin

instr AudioShapeChange
  kCycle timeinstk
  kColumns init 3
  if kCycle == 2 then
    kColumns = 2
    reinit RESIZE
  endif
RESIZE:
  aX[][] init 3, i(kColumns)
  rireturn
  aY[][] init 3, 3
  aAngles[][] = taninv2(aY, aX)
endin
</CsInstruments>
<CsScore>
i "ControlShapeChange" 0 .002
i "AudioShapeChange" .01 .002
e
</CsScore>
</CsoundSynthesizer>
