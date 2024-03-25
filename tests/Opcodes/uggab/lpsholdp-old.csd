<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpsholdp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
ipan  = p4
ktrig = 0   ; (no retriggering)
; kphase - looping pointer - is generated using a random spline
kphase rspline  0,1,0.5,5
; a loop sequence of midi note numbers and durations
;                      val dur val dur etc...
knote lpsholdp kphase, 60, 1,  59, 0.1, 62, 1, 64, 1, 67, 0.1, 65, 1
asig  gbuzz   0.2, cpsmidinn(knote), 30, 1, 0.5, 1 
      outs    asig*ipan, asig*(1-ipan)
endin

</CsInstruments>
<CsScore>
; cosine wave.
f 1 0 16384 11 1

; 2 layers of the loop are played each with a different pan position (p4)
i 1 0 60 0.25
i 1 0 60 0.75

e
</CsScore>
</CsoundSynthesizer>
