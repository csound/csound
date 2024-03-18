<CsoundSynthesizer>
<CsOptions>
-odac -d -m128
</CsOptions>
<CsInstruments>
;example by joachim heintz (& Menno Knevel)
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

ires system_i 1,{{ atsa fox.wav fox.ats }} ; default settings

giSine    ftgen     0, 0, 1024, 10, 1
gSfile    =         "fox.ats"
giNumParts ATSinfo  gSfile, 3 ;overall number of partials
giDur     ATSinfo   gSfile, 7 ;duration 
          seed      0
          

  instr PlayList
event_i "i", "PlayAll", 0, 1, 1, 0, .5 ;sine only, half speed
event_i "i", "PlayAll", giDur*2+1, 1, 0, 1, .5 ;noise only
event_i "i", "PlayAll", giDur*4+2, 1, .5, .5, .5 ;half sine, half noise
  endin

  instr PlayAll
iSinAmnt  =         p4 ;sinee amount (0-1)
iNzAmnt   =         p5 ;noise amount (0-1)
iSpeed    =         p6 ;speed
p3        =         giDur/iSpeed
ktime     line      0, giDur/iSpeed, giDur
          prints    "Resynthesizing all partials with tone = %.1f and noise = %.1f.\n", iSinAmnt, iNzAmnt
aOut      ATSsinnoi ktime, iSinAmnt, iNzAmnt, 1, gSfile, giNumParts
          outs      aOut, aOut
  endin

  instr PlayBand
iOffset   =         p4 ;offset in partials
iSpeed    =         p5 ;speed
p3        =         giDur/iSpeed
ktime     line      0, giDur/iSpeed, giDur
          prints    "Resynthesizing partials %d to %d with related noise.\n", iOffset+1, iOffset+10
aOut      ATSsinnoi ktime, 1, .3, 1, gSfile, 10, iOffset, 1 ; a bit less noise (.3)
          outs      aOut*2, aOut    ; left channel a bit louder
;call itself again
 if iOffset < giNumParts - 20 then
          event_i   "i", "PlayBand", giDur/iSpeed+1, 1, iOffset+10, iSpeed
 endif
  endin

  instr PlayWeighted
  ;sine amount, noise amount and speeed are varying
kSinAmnt  randomi   0, 1, 1, 3
kNzAmnt   =         1-kSinAmnt
kSpeed    randomi   .01, .3, 1, 3
async     init      0
atime, aEnd syncphasor kSpeed/giDur, async
kTrig     metro     100
kEnd      max_k     aEnd, kTrig, 1 ;1 if phasor signal crosses zero
ktime     downsamp  atime
aOut      ATSsinnoi ktime*giDur, kSinAmnt, kNzAmnt, 1, gSfile, giNumParts
          outs      aOut*.6, aOut   ; pan a bit to the right
  ;exit if file is at the end 
if kEnd == 1 then
           event     "e", 0, 0
endif

endin

</CsInstruments>
<CsScore>
i "PlayList" 0 1
i "PlayBand" 20 1 0 .5
i "PlayWeighted" 110 100
</CsScore>
</CsoundSynthesizer>
