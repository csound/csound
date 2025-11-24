<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>
; Intentionally malformed GEN line in the score to provoke fgens errors
instr 1
  a1 poscil 0.1, 440, 1
  outs a1, a1
endin
</CsInstruments>
<CsScore>
; GEN lines: put an intentionally incorrect/insufficient GEN line
f1 0 1024  ; missing required arguments for most GENs
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
