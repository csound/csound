<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ilen   mp3len p4        ;calculate length of mp3 file
print  ilen

asigL, asigR mp3in p4
       outs  asigL, asigR

endin
</CsInstruments>
<CsScore>

i 1 0 30 "beats.mp3"
e
</CsScore>
</CsoundSynthesizer>
