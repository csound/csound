<CsoundSynthesizer>
<CsInstruments>
sr=44100
ksmps=32
nchnls=2
0dbfs=1

opcode adsr140_calc_coef, k, kk
  
  knum_samps, kratio xin
  xout exp( -log((1.0 + kratio) / kratio) / knum_samps)

endop

instr 1

print exp( -1 / -1)
print exp( 0 / -1)
kval = adsr140_calc_coef(2, 4)

endin


</CsInstruments>

<CsScore>
i1 0 0.1 
</CsScore>
</CsoundSynthesizer>
