<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

instr 1
a1 oscili 0.5, 440
outs a1,a1
endin

</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>
