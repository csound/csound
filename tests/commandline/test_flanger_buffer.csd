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
  ksample init 0
  asignal = (ksample + 1) / 1000
  adelay = p5 / sr
  if ksample == 16 then
    reinit DELAY
  endif
DELAY:
  iskip = i(ksample) == 0 ? 0 : p6
  imax = (i(ksample) >= 16 && p7 != 0) ? p7 : p4
  aout flanger asignal, adelay, 0, imax / sr, iskip
  rireturn
  kout downsamp aout
  if ksample >= ceil(p5) then
    kexpected = (ksample - p5 + 1) / 1000
    if (p6 == 0 || p7 != 0) && ksample >= 16 && ksample < 16 + p5 then
      kexpected = 0
    endif
    if abs(kout - kexpected) > .000001 then
      printks "flanger mismatch: max=%g delay=%g sample=%g expected=%g actual=%g\n", 0, p4, p5, ksample, kexpected, kout
      exitnowk(-1)
    endif
  endif
  if ksample == 31 then
    gkchecks += 1
  endif
  ksample += 1
endin

instr 99
  if i(gkchecks) != 7 then
    prints "not all flanger buffer checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Maximum and requested delays are expressed in samples.
i 1 0 .004 0 0 1
i 1 0 .004 4 4 1
i 1 0 .004 4.5 4.5 1
i 1 0 .004 -4 4 1
; Clear the same allocation, grow it, and shrink it on reinit.
i 1 0 .004 4 4 0
i 1 0 .004 4 4 1 8
i 1 0 .004 8 4 1 4
i 99 .005 .001
</CsScore>
</CsoundSynthesizer>
