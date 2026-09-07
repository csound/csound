<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

; A non-power-of-two window table selects grain2's float-phase path.
giwave ftgen 1, 0, 16, 7, 1, 16, 1
giwindow ftgen 2, 0, 18, 7, 0, 18, 1
gkcount init 0

instr 1
  aout grain2 1, 0, 0.0001, 1, 1, 2, 1, 1, 8
  ksample downsamp aout
  if gkcount == 5 then
    ; After the first window, phase 1.041666... wraps to .041666...
    if abs(ksample - (1.0 / 24.0)) > 0.02 then
      printks "grain2 float window phase did not wrap: %.9f\n", 0, ksample
      exitnowk(-1)
    endif
  endif
  gkcount += 1
endin
</CsInstruments>
<CsScore>
i1 0 0.001
</CsScore>
</CsoundSynthesizer>
