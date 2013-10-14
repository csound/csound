<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

instr 1

  a1 oscil 10000, 440, 1

  out a1

endin

</CsInstruments>
<CsScore>

f1	0	4096	10	1	; use GEN10 to compute a sine wave

;ins	strt	dur

i1	0	4

e					; indicates the end of the score

</CsScore>
</CsoundSynthesizer>
