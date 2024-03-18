<CsoundSynthesizer>
<CsOptions>
-odac -d -m1
</CsOptions>
<CsInstruments>
;example by joachim heintz (& Menno Knevel)
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;ATSA wants a mono file!
ires system_i 1,{{ atsa -h.1 -c1 fox.wav fox.ats }} ; only 1 cycle and small hop size

instr AllTheNoise
Sfile    =        "fox.ats"
         prints   "Resynthesizing with all the noise.\n"
iDur     ATSinfo  Sfile, 7
p3       =        iDur
ktime    line     0, iDur, iDur
asig     ATSaddnz ktime, Sfile, 25
         outs     asig, asig

;start next instr
         event_i  "i", "NoiseInBandsOfFive", iDur+1, 1, 0
endin

instr NoiseInBandsOfFive
Sfile    =        "fox.ats"
         prints   "Resynthesizing with noise bands %d - %d.\n", p4, p4+5
iDur     ATSinfo  Sfile, 7
p3       =        iDur
ktime    line     0, iDur, iDur
asig     ATSaddnz ktime, Sfile, 5, p4
         outs     asig, asig

;start next instr
if p4 < 20 then
         event_i  "i", "NoiseInBandsOfFive", iDur+1, 1, p4+5
endif
endin
</CsInstruments>
<CsScore>
i "AllTheNoise" 0 1
e 25
</CsScore>
</CsoundSynthesizer>
