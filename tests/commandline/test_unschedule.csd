<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

nchnls = 2
0dbfs = 1

instr hello
exitnow(-1)
endin

instr 3
schedule 1,1,4,1,"hello"
schedule 1,2,4,1,"hello"
endin

instr 4
unschedule  hello,1,4,1,"hello"
unschedule  hello,2,4,1,"hello"
endin

schedule 3, 0, 0
schedule 4, 0.5, 0

</CsInstruments>
<CsScore>
f0 4
</CsScore>
</CsoundSynthesizer>


