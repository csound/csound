<CsoundSynthesizer>
<CsOptions>
-odac -d -m128
</CsOptions>
<CsInstruments>
;example by joachim heintz
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

ires1 system_i 1,{{ atsa fox.wav fox.ats }} ; default settings

giSine    ftgen     0, 0, 1024, 10, 1
gSfile    =         "fox.ats"
giNumParts ATSinfo  gSfile, 3 ;overall number of partials
giDur     ATSinfo   gSfile, 7 ;duration 
          seed      0


  instr ReadOnePartial
iPartial  =         p4
p3        =         giDur
ktime     line      0, giDur, giDur
          prints    "Resynthesizing partial number %d.\n", iPartial
kFq,kAmp  ATSread   ktime, gSfile, iPartial
kAmp      port      kAmp, .1 ;smooth amplitudes - still not satisfactoring
aOut      poscil    kAmp, kFq, giSine
aOut      linen     aOut, 0, p3, .01 ;anti-click
          outs      aOut*10, aOut*10

;start next instr: normal speed, three loops, pause between loops one second
          event_i   "i", "MasterRand", giDur+3, 1, 1, 3, 2
  endin

  instr MasterRand
  ;random selections of 10 partials per second, overlapping
iSpeed    =         p4 ;speed of reading / playing
iNumLoops =         p5 ;number of loops
iPause    =         p6 ;length of pause between loops
          prints    "Resynthesizing random partials.\n"
p3        =         (giDur/iSpeed+iPause) * iNumLoops
;start next instr: half speed, three loops, three seonds pause between loops
          event_i   "i", "MasterArp", p3+3, 1, .5, 3, 3
;loop over duration plus pause
loop:
          timout    0, giDur/iSpeed+iPause, play
          reinit    loop
play:
gkTime    line      0, giDur/iSpeed, giDur ;start time from 0 in each loop
kTrig     metro     10 ;10 new partials per second
 ;call subinstrument if trigger and no pause
 if kTrig == 1 && gkTime < giDur then
kPart     random    1, giNumParts+.999
          event     "i", "PlayRand", 0, 1, int(kPart)
 endif

  endin
  
  instr MasterArp
  ;argeggio-like reading and playing of partials 
iSpeed    =         p4 ;speed of reading / playing
iNumLoops =         p5 ;number of loops
iPause    =         p6 ;length of pause between loops
          prints    "Arpeggiating partials.\n"
p3        =         (giDur/iSpeed+iPause) * iNumLoops
loop:
          timout    0, giDur/iSpeed+iPause, play
          reinit    loop
play:
gkTime    line      0, giDur/iSpeed, giDur
kArp      linseg    1, (giDur/iSpeed)/2, giNumParts, (giDur/iSpeed)/2, 1 ;arp up and down
kTrig     metro     10 ;10 new partials per second
 if kTrig == 1 && gkTime < giDur then
          event     "i", "PlayArp", 0, 5, int(kArp)
 endif

;exit csound when finished
          event_i   "i", "End", p3+5, 1
  endin

  instr PlayRand
iPartial  =         p4
kFq,kAmp  ATSread   gkTime, gSfile, iPartial
kamp      port      kAmp, .15 ;smooth amplitudes
aOut      poscil    kAmp, kFq, giSine
aOut      linen     aOut, .01, p3, .01
          outs      aOut, aOut
  endin

  instr PlayArp
kCount    init      1 ;k-cycle
iPartial  =         p4
kFq,kAmp  ATSread   gkTime, gSfile, iPartial
 if kCount == 1 then ;get freq from first k-cycle
kModFq    =         kFq
  ;avoid to go with 0 Hz as this blocks the mode filter
  if kModFq == 0 then
          turnoff
  endif
 endif
iVol      random    -42, -12 ;db
iOffset   random    .01, .1 ;no too regularily ...
aImp      mpulse    ampdb(iVol), p3, iOffset
iQ        random    500, 5000
aOut      mode      aImp, kModFq, iQ
aOut      linen     aOut, 0, p3, p3/3
          outs      aOut, aOut
kCount    =         2
  endin

  instr End
          exitnow
  endin
</CsInstruments>
<CsScore>
i "ReadOnePartial" 0 1 10
e 999
</CsScore>
</CsoundSynthesizer>
