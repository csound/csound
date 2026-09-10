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
  ; Put one audible partial in a known bin, including the last bin.
  kframe[] init 130
  kframe[p4 * 2] = .1
  kframe[p4 * 2 + 1] = 440
  fsig pvsfromarray kframe, 32
  aout pvsadsyn fsig, p5, 1, p6, p7
  aref pvsadsyn fsig, 1, 1, p4
  kerror max_k abs(aout - aref * p8), 1, 1
  ksample downsamp aout
  if !(kerror <= .000001 && abs(ksample) <= .2) then
    printks "pvsadsyn selected the wrong bins\n", 0
    exitnowk(-1)
  endif
  kpeak peak aout
  kcycle init 0
  if kcycle == 14 then
    if p8 == 1 && !(kpeak > .00001) then
      printks "pvsadsyn omitted the selected bin\n", 0
      exitnowk(-1)
    endif
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 99
  if i(gkchecks) != 8 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Bin, count, offset, increment, whether the bin is selected.
i 1 0 .01 64 33 0 2 1
i 1 0 .01 63 33 0 2 0
i 1 0 .01 64 1 64 1 1
i 1 0 .01 64 2 63 1 1
i 1 0 .01 64 22 1 3 1
; Preserve integer truncation of fractional selection arguments.
i 1 0 .01 64 65.75 0 1 1
i 1 0 .01 64 33.75 -.5 2.75 1
i 1 0 .01 64 1 64.75 65.75 1
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
