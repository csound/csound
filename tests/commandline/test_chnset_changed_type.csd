<CsTest>
description = "chnset stops safely when a changed name refers to a string channel"

[expect]
exit = "nonzero"
stderr = ["channel already exists with incompatible type", "end of Performance"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --m-warnings=1
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

chnset "text", "x1"

instr 1
  kCycle timeinstk
  ; The first two writes succeed. The third must report the type mismatch
  ; and return before trying to write through the failed channel pointer.
  kPick = (kCycle >= 3 ? 1 : 0)
  SName sprintfk "x%d", kPick
  chnset kCycle, SName
endin
</CsInstruments>
<CsScore>
i 1 0 0.005
</CsScore>
</CsoundSynthesizer>
