<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
nchnls = 2

instr 1
a1 oscili 1,441,-1,0.25
a2 oscili 1,441
sinus:Complex[] quadosc 441
k1 rms real(sinus)-a1
k2 rms imag(sinus)-a2
if int(k1) > 0 || int(k2) > 0 then
  printks "quadrature sinusoid computation error\n", 1
  exitnowk(-1)
endif
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
