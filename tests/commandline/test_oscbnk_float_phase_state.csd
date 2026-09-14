<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
kr = 125
ksmps = 8
nchnls = 1
; A non-power-of-two table selects oscbnk's float-phase path.
giTable ftgen 1, 0, 100, -7, 0, 100, 1

gkBlock init 0
gkPrevious init 0
gkBad init 0

instr 1
  a1 oscbnk 100, 0, 0, 0, 1, 1234, 0.1, 0.2, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, 1, 0, 0, 0, 0, 0, 0
  kFirst downsamp a1
  if gkBlock > 0 then
    if kFirst != 0 then
      if abs(kFirst - gkPrevious) < 0.001 then
        gkBad = 1
      endif
    endif
  endif
  gkPrevious = kFirst
  gkBlock += 1
  if gkBlock >= 6 then
    turnoff
  endif
endin

instr 2
  if gkBad != 0 then
    printks "oscbnk float phase did not advance between blocks\n", 0
    exitnowk(1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.08
i 2 0.06 0.01
</CsScore>
</CsoundSynthesizer>
