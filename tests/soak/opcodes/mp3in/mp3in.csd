<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

iskptim	 = .3
ibufsize = 64
ar1, ar2 mp3in "beats.mp3", iskptim, 0, 0, ibufsize
         outs ar1, ar2

endin
</CsInstruments>
<CsScore>

i 1 0 2
e
</CsScore>
</CsoundSynthesizer>
