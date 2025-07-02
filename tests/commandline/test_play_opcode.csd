<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

instr 1
myInstrument:InstrDef = createinstr({{
out oscili(p4,p5)
}})
myInstance1:Instr = play(myInstrument, 0dbfs/4, 440)
myInstance2:Instr = play(myInstrument, 0dbfs/4, 330)
turnoff myInstance1, times:k() > 1 ? 1 : 0
endin

</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
