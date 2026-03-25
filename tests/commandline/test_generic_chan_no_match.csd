<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Expected failure test.
; Writes a generic Complex value to the software bus, then attempts to
; read the same channel through the primitive k-rate control-channel
; path. The incompatible channel/type access should fail and produce the
; nonzero exit status expected by test.py.

instr 1
 cvar:Complex init 1,1
 chnset cvar, "Test1"
 kvar chnget "Test1"
endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
