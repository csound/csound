<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
schedule 1,0,1
gicnt init 0
instr 1
i1 compilestr {{
instr name
 a1 oscili 1000, 440
 out a1
endin
schedule "name", 0, 1
}}
if gicnt < 10 then
schedule 1, 0, 1
gicnt += 1
else
event_i "e 0 0",0
endif

endin
</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>

