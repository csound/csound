<CsoundSynthesizer> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

gihand fiopen "test1.txt", 0 

instr 1

ires  random  0, 100
      fouti gihand, 0, 1, ires 
      ficlose gihand 

endin 
</CsInstruments> 
<CsScore> 

i 1 0 1 

e
</CsScore> 
</CsoundSynthesizer> 
