<CsTest>
description = "instance init, play, and perf pass optional p-fields correctly"

[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr Target
  chnset p4, "init-p4"
  chnset p5, "init-p5"
  kP4 = p4
  kP5 = p5
  chnset kP4, "p4"
  chnset kP5, "p5"
endin

instr 1
  auto:Instr = create(Target)
  iSplice = splice(auto, this, 0)
  kAuto4 = 100 + timeinstk()
  kAuto5 = 200 + timeinstk()
  kError perf auto, kAuto4, kAuto5
  kSeen4 chnget "p4"
  kSeen5 chnget "p5"
  if kError != 0 || kSeen4 != kAuto4 || kSeen5 != kAuto5 then
    exitnowk -1
  endif
  delete auto
endin

instr 2
  explicit:Instr = create(Target)
  iSplice = splice(explicit, this, 0)
  iError init explicit, 10, 20
  kError perf explicit, 30, 40
  kSeen4 chnget "p4"
  kSeen5 chnget "p5"
  if iError != 0 || kError != 0 || kSeen4 != 30 || kSeen5 != 40 then
    exitnowk -1
  endif
  delete explicit
endin

instr 3
  played:Instr = play(Target, 50, 60)
  iSeen4 chnget "init-p4"
  iSeen5 chnget "init-p5"
  if iSeen4 != 50 || iSeen5 != 60 then
    exitnow -1
  endif
  delete played
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 .02 .01
i 3 .04 .01
e
</CsScore>
</CsoundSynthesizer>
