<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1


instr 1
myinstr:InstrDef = createinstr("out oscili(p4,p5)")
schedule(myinstr, 0, 1, 0.5, 440)
delete myinstr   // strictly deinit time
endin



</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
