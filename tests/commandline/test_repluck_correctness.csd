<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  kcycle init 0
  aout wgpluck2 p4, .2, 440, .5, .5
  ksample downsamp aout
  kpeak max_k abs(aout), 1, 1
  if p4 == 0 && !(ksample == 0 && kpeak == 0) then
    printks "zero pluck point produced output: sample=%g peak=%g\n", 0, ksample, kpeak
    exitnowk(-1)
  elseif !(ksample >= -1 && ksample <= 1 && kpeak >= 0 && kpeak <= 1) then
    printks "non-finite endpoint pluck output: pluck=%g sample=%g peak=%g\n", 0, p4, ksample, kpeak
    exitnowk(-1)
  endif
  if kcycle == 29 then
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 2
  ipluck = sqrt:i(-1)
  kpickup = sqrt(-1)
  kreflect = sqrt(-1)
  aout wgpluck2 ipluck, .2, 440, kpickup, kreflect
  ksample downsamp aout
  if !(ksample >= -1 && ksample <= 1) then
    printks "invalid positions produced non-finite output\n", 0
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 3
  kcycle init 0
  kamp = (kcycle < 2 ? 0 : 1)
  aexcite = (kcycle < 2 ? 1 : 0)
  aout repluck 0, kamp, 440, .5, .5, aexcite
  ksample downsamp aout
  kpeak max_k abs(aout), 1, 1
  if !(ksample == 0 && kpeak == 0) then
    printks "zero-amplitude repluck output: sample=%g peak=%g\n", 0, ksample, kpeak
    exitnowk(-1)
  endif
  if kcycle == 2 then
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 4
  kfrequency = sqrt(-1)
  asignal = 0
  aout streson asignal, kfrequency, .5
  ksample downsamp aout
  if !(ksample == 0) then
    printks "invalid streson frequency produced non-finite output\n", 0
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

; The peak must stay inside the fixed ends; pickup 0 and 1 wrap alike.
instr 5
  aend wgpluck2 p4, .2, 440, p5, .5
  aref wgpluck2 .995, .2, 440, 0, .5
  kerror max_k abs(aend - aref), 1, 1
  ksample downsamp aend
  if !(kerror <= .000001 && ksample >= -1 && ksample <= 1) then
    printks "endpoint pluck or pickup differs from the last interior peak\n", 0
    exitnowk(-1)
  endif
  kcycle init 0
  if kcycle == 29 then
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 99
  if i(gkchecks) != 10 then
    prints "not all repluck correctness checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02 0
i 1 0 .02 .999
i 1 0 .02 1
i 2 0 .001
i 3 0 .002
i 4 0 .001
i 5 0 .02 .999 0
i 5 0 .02 1 0
i 5 0 .02 .999 1
i 5 0 .02 1 1
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
