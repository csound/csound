<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
nchnls = 1
0dbfs = 1

instr 1
 icps cpsmidi
 print icps
 out oscili(0.1, icps)
endin

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>


