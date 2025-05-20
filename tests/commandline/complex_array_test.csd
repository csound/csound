<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1

instr 1
 sig:Complex[] hilbert oscili(p4,p5)
 mod:Complex[] = oscili(0.5,100,-1,0.25), oscili(0.5,100,-1)
 ssb:Complex[] = mod * sig
   out real(ssb)
endin

</CsInstruments>
<CsScore>
i1 0 1 0.5 400
</CsScore>
</CsoundSynthesizer>

