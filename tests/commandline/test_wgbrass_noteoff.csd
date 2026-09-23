<CsTest>
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
nchnls = 1
0dbfs = 1
gaFinite init 0
gaHeld init 0

instr 1
  gaFinite wgbrass .5, 440, .5, .1, 0, 0
endin
instr 2
  gaHeld wgbrass .5, 440, .5, .1, 0, 0
endin
instr 3
  kTime timeinsts
  kPeak peak gaFinite
  kDifference peak gaFinite-gaHeld
  kChecked init 0
  if kTime > .33 && kChecked == 0 then
    if !(kPeak > .001 && kDifference < .000001) then
      printks "wgbrass note-off differs from scheduled release\n", 0
      exitnowk(-1)
    endif
    kChecked = 1
  endif
endin
</CsInstruments>
<CsScore>
; The finite note releases at .35-.1; the held note receives note-off at .25.
i 1 0 .35
i 2 0 -1
i 3 0 .34
i -2 .25 0
</CsScore>
</CsoundSynthesizer>
