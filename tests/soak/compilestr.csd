<CsoundSynthesizer>
<CsOptions>
-o dac -d
</CsOptions>
<CsInstruments>
sr = 44100
nchnls = 1
ksmps = 32
0dbfs = 1

instr 1

 ;will fail because of wrong code
ires compilestr {{
instr 2 
a1 oscilb p4, p5, 0
out a1 
endin 
}}
print ires ; returns -1 because not successfull

 ;will compile ...
ires compilestr {{
instr 2 
a1 oscils p4, p5, 0
out a1 
endin 
}}
print ires ; ... and returns 0

 ;call the new instrument
 ;(note that the overall performance is extended)
scoreline_i "i 2 0 3 .2 415"

endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

