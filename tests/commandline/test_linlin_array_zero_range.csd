<CsTest>
description = "linlin reports zero-width ranges at the calling rate and preserves output"
[expect]
exit = "nonzero"
# Csound numbers the four named instruments in their definition order.
# Require the opcode error itself; a deleted-note message is not enough.
stderr = [
  "linlin: Division by zero",
  "INIT ERROR in instr 1 (opcode linlin)",
  "INIT ERROR in instr 2 (opcode linlin)",
  "PERF ERROR in instr 3 (opcode linlin)",
  "PERF ERROR in instr 4 (opcode linlin)"
]
output = [
  "InitMap: output unchanged",
  "InitBlend: output unchanged",
  "ControlMap: output unchanged",
  "ControlBlend: output unchanged"
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giOutput[] fillarray 7, 8, 9
gkOutput[] fillarray 7, 8, 9
instr InitMap
  iInput[] fillarray 0, 0.5, 1
  ; Equal input endpoints would divide by zero.
  giOutput linlin iInput, 10, 20, 1, 1
endin

instr InitBlend
  iInput[] fillarray 0, 0.5, 1
  iOther[] fillarray 10, 20, 30
  ; Equal input endpoints would divide by zero.
  giOutput linlin 0.5, iInput, iOther, 1, 1
endin

instr ControlMap
  kInput[] fillarray 0, 0.5, 1
  ; Equal input endpoints would divide by zero.
  gkOutput linlin kInput, 10, 20, 1, 1
endin

instr ControlBlend
  kInput[] fillarray 0, 0.5, 1
  kOther[] fillarray 10, 20, 30
  ; Equal input endpoints would divide by zero.
  gkOutput linlin 0.5, kInput, kOther, 1, 1
endin

instr CheckOutput
  ; An error must leave both the values and the output length unchanged.
  if lenarray(giOutput) == 3 && \
      giOutput[0] == 7 && \
      giOutput[1] == 8 && \
      giOutput[2] == 9 && \
      lenarray(gkOutput) == 3 && \
      gkOutput[0] == 7 && \
      gkOutput[1] == 8 && \
      gkOutput[2] == 9 then
    SCase strget p4
    printf "%s: output unchanged\n", 1, SCase
  endif
endin
</CsInstruments>
<CsScore>
i "InitMap" 0 .015625
i "CheckOutput" 0.03125 .015625 "InitMap"
i "InitBlend" 0.0625 .015625
i "CheckOutput" 0.09375 .015625 "InitBlend"
i "ControlMap" 0.125 .015625
i "CheckOutput" 0.15625 .015625 "ControlMap"
i "ControlBlend" 0.1875 .015625
i "CheckOutput" 0.21875 .015625 "ControlBlend"
e
</CsScore>
</CsoundSynthesizer>
