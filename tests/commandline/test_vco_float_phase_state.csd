<CsoundSynthesizer>
<CsOptions>
-d -m0 -n
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 8
nchnls = 1
0dbfs = 1

; A non-power-of-two table selects vco's floating phase path.
gisine ftgen 1, 0, -100, 7, 0.5, 100, 1.5

instr 1
  a1 vco 1, 400, 1, .5, gisine, .01, .000001, 500, .25
  kfirst downsamp a1
  kblock init 0
  if kblock == 0 then
    gkfirst = kfirst
  elseif kblock == 1 then
    if abs(kfirst - gkfirst) < 0.00001 then
      printks "vco float phase did not advance between blocks: %f %f\\n", 1, gkfirst, kfirst
      exitnowk(-1)
    endif
  endif
  kblock += 1
endin
</CsInstruments>
<CsScore>
i1 0 .024
e
</CsScore>
</CsoundSynthesizer>
