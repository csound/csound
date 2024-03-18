<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

0dbfs = 1

instr 1
 gkamp = p4
 gkfr = p5
 gkgate = 1
 gkatt = p6
 gkdec = p7
 gksus = p8
 gkrel = p9
endin

instr 2
 a1 oscili gkamp,gkfr
 a2 gtadsr  a1,gkatt,gkdec,gksus,gkrel,gkgate
 out a2
 gkgate = 0;
endin

</CsInstruments>
<CsScore>
i1 0 1 1 440 0.1 0.1 0.5 0.1
i1 1.2 1 1 660 0.01 0.1 0.2 0.3
i1 2.5 1 1 550 0.01 0.1 0.7 1
i2 0 5
</CsScore>
</CsoundSynthesizer>