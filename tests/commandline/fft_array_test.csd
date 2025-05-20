<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

nchnls = 1
0dbfs = 1

instr 1
 kArr[] genarray_i 0,1023
 spec:Complex[] fft kArr
 kArr fft spec
endin

</CsInstruments>
<CsScore>
i1 0 1 
</CsScore>
</CsoundSynthesizer>


