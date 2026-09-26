<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1

gfInput pvsinit 64, 1, 64, 1
gfOutput pvsinit 64, 1, 64, 1

instr 1
  ; Keep the source running before and after the short pvsgendy note.
  aTone oscili .5, 1024
  gfInput pvsanal aTone, 64, 1, 64, 1
endin

instr 2
  ; Zero controls copy each active input spectrum.
  gfOutput pvsgendy gfInput, 0, 0
endin

instr 3
  ; Observe the whole block, including the samples after instrument 2 ends.
  aBefore, aBeforeFrequency pvsbin gfInput, 8
  aAfter, aAfterFrequency pvsbin gfOutput, 8
  kCycle timeinstk
  if kCycle <= 3 then
    kSample = 0
    while kSample < ksmps do
      kAbsolute = (kCycle - 1) * ksmps + kSample
      kBefore vaget kSample, aBefore
      kAfter vaget kSample, aAfter
      if kAbsolute < 5 || kAbsolute >= 45 then
        if kAfter != 0 then
          printf "pvsgendy left data outside its note at sample %d\n", 1, kAbsolute
          exitnowk(-1)
        endif
      elseif kAfter != kBefore then
        printf "pvsgendy changed an active sample with zero controls\n", 1
        exitnowk(-1)
      endif
      kSample += 1
    od
  endif
endin
</CsInstruments>
<CsScore>
; Instrument 2 spans samples 5 through 44.
i1 0 0.0078125
i2 0.0006103515625 0.0048828125
i3 0 0.0078125
e
</CsScore>
<CsTest>
description = "pvsgendy copies active sliding samples and clears samples outside its note"
[expect]
exit = 0
</CsTest>
</CsoundSynthesizer>
