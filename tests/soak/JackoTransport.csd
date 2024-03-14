<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr  = 48000
ksmps = 128
nchnls  = 2
0dbfs   = 1

; by Menno Knevel - 2023

JackoInit   "default", "csound6"                                        ; Csound as a Jack client

instr 1	; position & start external DAW
JackoTransport  3, 10                                                   ; set playbackhead of DAW to zero in 10 seconds                                                    
JackoTransport  1                                                       ; start transport and thus the playbackhead
endin

instr 2	; stop playback of DAW
JackoTransport  2
endin

</CsInstruments>
<CsScore>
i1  5	.1
i2  20  .1
e
</CsScore>
</CsoundSynthesizer>
