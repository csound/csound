<CsoundSynthesizer>
<CsOptions>
-odac -d -m1
</CsOptions>
<CsInstruments>
;example by joachim heintz (& Menno Knevel)
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

;ATSA wants a mono file!
ires system_i 1,{{ atsa -h.1 -c2 fox.wav fox.ats }} ; only 2 cycles and small hop size

giSine    ftgen     0, 0, 1024, 10, 1

  instr AllTheTones
Sfile     =         "fox.ats"
          prints    "Resynthesizing with all the tones.\n"
iDur      ATSinfo   Sfile, 7
p3        =         iDur
iNumParts ATSinfo   Sfile, 3
          prints    "Overall number of partials = %d\n", iNumParts
ktime     line      0, iDur, iDur
asig      ATSadd    ktime, 1, Sfile, giSine, iNumParts
          outs      asig, asig

;start next instr
          event_i   "i", "TonesInBandsOfTen", iDur+1, iDur, 0, iNumParts
  endin

  instr TonesInBandsOfTen
Sfile     =         "fox.ats"
iOffset   =         p4 ;start at this partial
iNumParts =         p5 ;overall number of partials
          prints    "Resynthesizing with partials %d - %d.\n", iOffset+1, iOffset+10
ktime     line      0, p3, p3
asig      ATSadd    ktime, 1, Sfile, giSine, 10, iOffset
          outs      asig, asig

;start next instance until there are enough partials left
 if iOffset+20 < iNumParts then
          event_i   "i", "TonesInBandsOfTen", p3+1, p3, iOffset+10, iNumParts
          else
          event_i   "i", "End", p3, 1
 endif
  endin

  instr End
          exitnow
  endin
</CsInstruments>
<CsScore>
i "AllTheTones" 0 1
e 999
</CsScore>
</CsoundSynthesizer>
