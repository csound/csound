<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; A builder and a definition share the numeric handle representation, but the
; registry type is authoritative. stminstance must reject a builder handle.
builder@global:i = stmcreate()
stmaddnode(builder, "A")
invalid_runner@global:i = stminstance(builder)
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
