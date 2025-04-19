<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
exitnow(-1)
endin

instr 3
schedule 1,1,4,1
endin

instr 4
unschedule 1,1,4,1
endin

schedule 3, 0, 0
schedule 4, 0.5, 0

</CsInstruments>
<CsScore>
f0 4
</CsScore>
</CsoundSynthesizer>


