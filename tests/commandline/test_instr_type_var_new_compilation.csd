<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

ires compilestr {{
instr Ss
 print p1
endin
 schedule(Ss,0,0)
}}

ires compilestr {{
 schedule(Ss,0,0)
}}

</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>
