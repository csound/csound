<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
giExcite ftgen 0, 0, 16, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
gaExcite init 0
gkpeak init 0

instr 1
  kcycle init 0
  if kcycle == 0 then
    aphase phasor sr / 16
    gaExcite table aphase * 16, giExcite
  else
    gaExcite = 0
  endif
  kcycle += 1
endin

instr 2
  aout repluck 0, 1, 440, .5, .5, gaExcite
  kpeak max_k abs(aout), 1, 1
  if kpeak > gkpeak then
    gkpeak = kpeak
  endif
endin

instr 99
  if !(i(gkpeak) > .000001) then
    prints "repluck missed excitation at the note start offset\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
i 2 .001875 .01
i 99 .015 .001
</CsScore>
</CsoundSynthesizer>
