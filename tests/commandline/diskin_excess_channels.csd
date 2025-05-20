<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>

nchnls = 2
0dbfs = 1

instr 2
 // mono file with 2 outputs (second out is zero)
 asig,asig1 diskin2 "fox.wav", p4
 out asig,asig1
 endin 
schedule 2,0,4,1

</CsInstruments>
<CsScore>
f0 4
</CsScore>
</CsoundSynthesizer>


