<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

   kfreq = 200
   ihrd = 0.5
   ipos = p4
   kvibf = 6.0
   kvamp = 0.3

asig gogobel .9, kfreq, ihrd, ipos, 1, 6.0, 0.3, 2
     outs asig, asig
endin
</CsInstruments>
<CsScore>
;audio file
f 1 0 256 1 "marmstk1.wav" 0 0 0
;sine wave for the vibrato
f 2 0 128 10 1

i 1 0.5 0.5 0.01
i 1 + 0.5 0.561
i 1 + 0.5 0.9
e
</CsScore>
</CsoundSynthesizer>
