<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -m0  --midi-key=4 --midi-velocity-amp=5 ; treat p4 and p5 as midi data
; For Non-realtime ouput leave only the line below:
; -o madsr-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021

instr 1	                              ; use score and treat p4 and p5 as midi data
icps    = cpsmidinn(p4)               ; take midi note (p4) from score
iveloc  ampmidid p5, 92               ; take velocity (p5) from score
;               att, dec, lvl, release
kenv    madsr  .001, .2,   1,    p6 
prints  "duration of note (%ds) + release (%2.1fs)\\n", p3, p6                ; 
asig    vco2    iveloc, icps
asig    butlp   asig, 2000 
        outs    asig*kenv, asig*kenv
endin

</CsInstruments>
<CsScore>
;      note vel release	
s
i 1 0 1 62  60  0
i 1 2 1 62  80  0	
i 1 4 1 62  100 0	
i 1 6 1 58  50  0
s
i 1 1 1 62  60  0
i 1 3 1 62  80  .5	
i 1 5 1 62  100 1.5	
i 1 7 1 58  50  3
e
</CsScore>
</CsoundSynthesizer>