<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

 asig diskin "fox.wav",1,0,1
 air diskin "drumsMlp.wav",1,0,1
 air buthp air/0dbfs,1000
 k1 linseg 0,p3/3,0,0,1,2*p3/3,1
 a1 oscili k1, 0.5, 1
 a2 oscili k1, 0.6, 1
 asig tvconv asig,air,1-a2,1-a1,256,1024
 asig clip asig,1,0dbfs
 outs asig, asig
   
endin

</CsInstruments>
<CsScore>
f1 0 1024 7 0 512 0 1 1 511 1
i1 0 30
e
</CsScore>
</CsoundSynthesizer>
