<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  kcount init 0
  kinput = kcount + 1
  ain upsamp kinput
  aout follow ain, p4
  kout downsamp aout

  if kcount == p5 - 2 && kout != 0 then
    exitnowk(-1)
  elseif kcount == p5 - 1 && kout != p5 then
    exitnowk(-1)
  elseif kcount == 2 * p5 - 1 then
    if kout != 2 * p5 then
      exitnowk(-1)
    endif
    gkchecks += 1
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 6 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 .0005 4
; Keep fractional-sample truncation and float-build rounding.
i 1 0 .03 .0006 4
i 1 0 .03 .01 80
; Zero, negative, and sub-sample periods retain the one-second fallback.
i 1 0 2.01 0 8000
i 1 0 2.01 -1 8000
i 1 0 2.01 .00001 8000
i 99 2.02 .001
</CsScore>
</CsoundSynthesizer>
