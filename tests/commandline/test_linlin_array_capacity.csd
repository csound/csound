<CsTest>
description = "linlin stops before writing when either array form outgrows its output"
[expect]
exit = "nonzero"
stderr = ["Array too small"]
output = ["Both outputs unchanged"]
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
gkMapped[] init 0
gkBlended[] init 0
instr Map
  kInput[] fillarray 0, 0.5, 1
  trim_i kInput, 1
  kCycle timeinstk
  kInput[0] = kCycle - 1
  gkMapped linlin kInput, 10, 20
  ; Cycle one writes [10]. Cycle two needs three slots, but only one exists.
  ; Its changed first input makes even a partial write visible.
  trim kInput, 3
endin

instr Blend
  kFirst[] fillarray 0, 0.5, 1
  kSecond[] fillarray 10, 20, 30
  trim_i kFirst, 1
  trim_i kSecond, 1
  kCycle timeinstk
  kFirst[0] = kCycle - 1
  gkBlended linlin 0.5, kFirst, kSecond
  ; Cycle one writes [5]. The next blend must stop before changing it to 5.5.
  trim kFirst, 3
  trim kSecond, 3
endin

instr CheckOutput
  if lenarray(gkMapped) == 1 && \
      gkMapped[0] == 10 && \
      lenarray(gkBlended) == 1 && \
      gkBlended[0] == 5 then
    printks "Both outputs unchanged\n", 0
  endif
endin
</CsInstruments>
<CsScore>
i "Map" 0 .03125
i "Blend" 0 .03125
i "CheckOutput" .03125 .015625
e
</CsScore>
</CsoundSynthesizer>
