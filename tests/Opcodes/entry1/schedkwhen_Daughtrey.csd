<CsoundSynthesizer>
<CsOptions>
-odac
; output for writing audio file with
; Android version of Csound:
;-o/sdcard/TableArps.wav
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; initialize global reverb channels
gaRvbL      init  0
gaRvbR      init  0
; initialize Count at -1 so Note tables are read starting at index 0
giCount     init -1
; arpeggio note tables
giMinAdd2   ftgen   1, 0, 0, 2, 60, 62, 63, 67, 72, 
                    74, 75, 74, 72, 67, 63, 62
giMajAdd2   ftgen   2, 0, 0, 2, 60, 62, 64, 67, 72, 
                    74, 76, 74, 72, 67, 64, 62

    instr Trigger
kTrig metro 8 
  schedkwhen  kTrig, 0, 0, "Arps", 0, 1/3, p4, p5, p6 ; trigger a note  
    endin

    instr Arps
giCount = giCount + 1 
iNote = table(giCount, p6, 0, 0, 1) ;read incrementally from the table
kEnv  = linsegr(0, 0.003, p5, p3, p5, .5, 0) ; amplitude envelope
iRnd  = random(0.92, 0.98) ; randomize amp and pluck point
iDtn  = random(-0.05, 0.05) ; random detune
iFreq = cpsmidinn(iNote + p4 + iDtn) ; convert note table values to cps 
aSig  = wgpluck2(iRnd, kEnv/5*iRnd, iFreq, .25, 0.06) ; plucked string opcodr
kEnv2 = linseg(.1, p3/2, 0)                                          
aSig2 = oscil(kEnv2*.1*iRnd, iFreq + 12) ; sine osc one octave higher
aMixL = aSig+aSig2/2
aMixR = aSig+aSig2
gaRvbL  +=  aSig*.22 ; send to global reverb channels
gaRvbR  +=  aSig*.22
  outs(aMixL, aMixR)
    endin
                     
    instr Reverb
aRvbL,aRvbR reverbsc gaRvbL, gaRvbR, 0.91, 11000
  outs(aRvbL,aRvbR)
   clear(gaRvbL,gaRvbR)
    endin

</CsInstruments>
<CsScore>
i"Reverb" 0 34
; for instr Trigger: p4=number of semi-tones 
; away from original note table freqs, 
; p5=amp, p6=table used
i"Trigger" 0 5 0  .8  1
i .  5   .    -4  .8  2
i . 10   .    -7  .8  1
i . 15   5.1  -2  .8  2

i . 22   1   -12  .8  1
i . 23   .    -9  .8  2
i . 24   .    -7  .8  1
i . 25   .    -4  .8  2
i . 26   .    -2  .8  2
i . 27   2     0  .8  1
i . 29   0.7   3  .8  2
e
</CsScore>
</CsoundSynthesizer>
; example by Scott Daughtrey
; inspired by example 01D13 by Iain McCurdy
; from the FLOSS manual
