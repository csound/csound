<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; looping back and forth,  0.05 crossfade 
kst  line     .2, p3, 2 ;vary loopstartpoint
aout flooper2 .8, 1, 0, kst, 0.05, 1, 0, 2  
     outs     aout, aout

endin
</CsInstruments>
<CsScore>
; Its table size is deferred,
; and format taken from the soundfile header
f 1 0 0 1 "fox.wav" 0 0 0

i 1 0 12
e
</CsScore>
</CsoundSynthesizer>
