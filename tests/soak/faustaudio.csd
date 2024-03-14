<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
instr 1
 a1 rand 0dbfs/4
 a2 oscili 0dbfs/4, 440
 ihandle faustcompile "process=+;", "-vec -lv 1"
 idsp,asig faustaudio ihandle,a1,a2
   out asig
endin
</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
