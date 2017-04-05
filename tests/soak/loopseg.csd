<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loopseg.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
kfreq  init     p4  ; frequency of loop repetition
ifrac  =        p5  ; frequency ratio of restart triggers
ktrig  metro    kfreq * ifrac  ; triggers to restart loop
iphase =        0   ; initial phase
; loop of note values (some glissandi)
;                                    val dur val dur etc...
knote loopseg  kfreq, ktrig, iphase, 40, 1,  40, 0,  43,1,43,0, 49,2,48,0, \
 47,1,47,0, 46,1,46,0, 46,1,47,0, 49,1,49,0, 43,1,43,0, 46,1,46,0, 40,1,39,0    
; loop of filter cutoff values (oct format). This loop, half speed of note loop.
kcfoct loopseg  kfreq*0.5, ktrig, iphase, 11,2,4,0, 12,1,4,0, 13,1,4,0, \
 11.5,3,4,0, 12.5,1,4,0, 13,2,4,0, 12.5,1,4,0
kenv  linseg   0,0.01,1,p3-5.01,1,5,0
ioct  =        int((rnd(0.999)*4)-2) ; random value either -1, 0 or 1
asig  vco2     0.2*kenv,cpsmidinn(knote)*octave(ioct),0 ; sawtooth
asig  moogladder  asig,cpsoct(kcfoct),rnd(0.6)          ; filter sawtooth
aL,aR pan2     asig,rnd(1)  ; random static pan location
      outs     aL, aR
endin

</CsInstruments>
<CsScore>

; 4 layers, each with a different frequency of loop repetition (p4),
;  frequency ratio of restart triggers (p5) and pan position (p6).
i 1  0 30 0.5   [11/19]
i 1  6 30 0.25  [11/13]
i 1 12 30 0.125 [11/16]
i 1 18 30 1     [11/12]
e
</CsScore>
</CsoundSynthesizer>
