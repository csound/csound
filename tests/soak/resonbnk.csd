<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

gifw ftgen 0,0,1024,20,2,1
instr 1
a1 diskin "fox.wav",1,0,1
kcfs[],krms,kerr,kcps lpcanal a1,1,512,1024,50,gifw
a2 rand 0dbfs
kpar[] apoleparams kcfs
kmin expseg 8000,p3/4,200,3*p3/4,600
a3 resonbnk a2*krms*kerr/4,kpar,kmin,16000-kmin,512,1
  out a3
endin



</CsInstruments>
<CsScore>

i1 0 32
</CsScore>
</CsoundSynthesizer>

