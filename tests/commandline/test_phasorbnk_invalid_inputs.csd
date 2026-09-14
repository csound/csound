<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
instr 1
  kPhase phasorbnk 100, 0, p4
endin
instr 2
  kPhase phasorbnk 100, p4, 2
endin
instr 3
  aPhase phasorbnk 100, p4, 2
endin
instr 4, 5
  kCount init 4
  kCycle init 0
  if kCycle == 2 then
    kCount = 2
    reinit RESIZE
  endif
RESIZE:
  if p1 == 4 then
    kPhase phasorbnk 100, 3, i(kCount)
  else
    aPhase phasorbnk 100, 3, i(kCount)
  endif
  rireturn
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 1e20
i 2 .1 .01 -1
i 2 .1 .01 2
i 2 .1 .01 1e20
i 3 .1 .01 -1
i 3 .1 .01 2
i 3 .1 .01 1e20
i 4 .2 .01
i 5 .2 .01
e
</CsScore>
</CsoundSynthesizer>
