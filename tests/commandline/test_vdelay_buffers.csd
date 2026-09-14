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
  ksignal = ksample / 1000
  asignal = ksignal
  kdelay = p5 * 1000 / sr
  adelay = kdelay
  ak vdelay asignal, kdelay, p4 * 1000 / sr
  aa vdelay asignal, adelay, p4 * 1000 / sr
  ak3 vdelay3 asignal, kdelay, p4 * 1000 / sr
  aa3 vdelay3 asignal, adelay, p4 * 1000 / sr
  kk downsamp ak
  ka downsamp aa
  kk3 downsamp ak3
  ka3 downsamp aa3
  ; A ramp has the same value under linear and cubic interpolation.
  ; Cubic cases use delays whose four taps precede the write position.
  if ksample > p4 + 8 then
    kexpected = (ksample - p6) / 1000
    if !(abs(kk-kexpected) < .00001 && abs(ka-kexpected) < .00001 && abs(kk3-kexpected) < .00001 && abs(ka3-kexpected) < .00001) then
      printks "vdelay mismatch: max=%g delay=%g expected=%g k=%g a=%g k3=%g a3=%g\n", 0, p4, p5, kexpected, kk, ka, kk3, ka3
      exitnowk(-1)
    endif
  endif
  if ksample == 100 then
    gkchecks += 1
  endif
  ksample += 1
endin

instr 99
  if i(gkchecks) != 12 then
    prints "not all vdelay buffer checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Maximum delay, requested delay, effective delay (all in samples).
i 1 0 .02 0 0 0
i 1 0 .02 .5 .25 0
i 1 0 .02 1 .5 0
i 1 0 .02 2 .5 .5
i 1 0 .02 3 .5 .5
i 1 0 .02 4 1.5 1.5
i 1 0 .02 8 1.5 1.5
i 1 0 .02 8 25.5 1.5
i 1 0 .02 8 -6.5 1.5
; These delays must wrap before conversion to a sample index.
i 1 0 .02 8 1099511627776 0
i 1 0 .02 8 -1099511627776 0
i 1 0 .02 8 8 0
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
