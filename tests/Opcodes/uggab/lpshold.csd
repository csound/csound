<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpshold.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
kfrq  init    p4  ; frequency of the loop 
ifrac =       p5  ; fraction of frequency at which to force retrigger
ipan  =       p6  ; pan position
ktrig metro   kfrq * ifrac  ; trigger to force restart the loop
iphs  =       0   ; initial phase of the loop
; a loop of midi note numbers:   note duration etc...
knote lpshold kfrq, ktrig, iphs, 61,  0.0625,  60, 0.9375,   61, 1, 58, 1, \
63, 2, 65, 3
aenv  linseg 0,0.01,1,p3-0.11,1,0.1,0   ; amplitude envelope
krnd  rspline -0.05,0.05,0.5,1 ; random detune
asig  gbuzz   0.2*aenv, cpsmidinn(knote+krnd), 30, 1, 0.5, 1 
      outs    asig*ipan, asig*(1-ipan)
endin

</CsInstruments>
<CsScore>
; cosine wave.
f 1 0 16384 11 1

; 3 layers of the loop are played, each at a different speed, 
; - with different retriggering rate, and pan location.
i 1 0 60 0.5   [8/10] 0.5
i 1 0 60 0.375 [8/11] 0.1
i 1 0 60 0.25  [8/13] 0.9
e
</CsScore>
</CsoundSynthesizer>
