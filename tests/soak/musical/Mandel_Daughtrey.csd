<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc for RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o Mandel_Daughtrey.wav -W ;;; for file output any platform 
</CsOptions>
<CsInstruments>

; example by Scott Daughtrey [ST Music]

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
    
seed 0       ; different random result at each run

gaRvbL  init ; initialize global reverb
gaRvbR  init
; table is 5th mode of Hungarian Minor Scale
giNotes ftgen 0, 0, 0, 2, 6.07, 6.08, 6.11, 7.00, 7.02, 7.03, 7.06, \  
        7.07, 7.08, 7.11, 8.00, 8.02, 8.03, 9.06, \
        8.07, 8.08, 8.11, 9.00, 9.02, 9.03, 9.06, \
        9.07

    instr Trigger 
kTrig = gausstrig(1, 1.85, 1.5) ; randomly trigger mandel opcode and timing of note production
kX    = trandom(kTrig, -1.6, 0) ; randomly select X coordinates
kY    = randomh(0, 1.2, 10) ; randomly select Y coordinates
kIter, kOutrig mandel  kTrig, kX, kY, 21 ; set range of iterations between 0-21
  schedkwhen kOutrig, 0, 0, "Mandel", 0, .7, p4, kIter ; trigger Mandel instr and send relevant parameters
    endin

    instr Mandel
iNote = table(p5, giNotes, 0, 0, 0) ; table uses mandel iterations to select indexed notes from giNotes ftable
iFreq = cpspch(iNote+p4) ; convert notes to cps
 if iFreq == 10.07 then
  iFreq = 7.07
 endif
iRand = random(.5, .99) ; randomize amplitude and pluck position (iPlk)
aSig  = wgpluck2(iRand, .3*iRand, iFreq, .2, .3)
iPan  = random(.2, .8) ; randomize pan position
aL, aR  pan2 aSig, iPan ; pan each note
  outs aL, aR ; output stereo signal
gaRvbL  +=  aL*.3 ; send a little to global reverb channels
gaRvbR  +=  aR*.3
    endin

    instr Reverb ; reverb instrument
aL, aR reverbsc gaRvbL, gaRvbR, .93, 2^14
  outs aL, aR
   clear gaRvbL, gaRvbR
    endin

</CsInstruments>
<CsScore>
i"Trigger" 0 3600 0
i .       .1 3600 1
i"Reverb"  0 3606
e
</CsScore>
</CsoundSynthesizer>