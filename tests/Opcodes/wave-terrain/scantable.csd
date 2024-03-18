<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac --limiter=0.95;;;realtime audio out & limiter
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o scantable.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

instr 1	

initial ftgen 1, 0, p5, 10, 1                         ; initial position = sine wave
imass   ftgen 2, 0, p5, -7, .1, p5, 1                 ; masses
istiff  ftgen 3, 0, p5, -7, 0, p5*.3, 0.8*p5, p5*.7, 0   ; stiffness
idamp   ftgen 4, 0, p5, -7, 1, p5, 1                  ; damping
ivelo   ftgen 5, 0, p5, -7, 0, p5, 0.5                ; initial velocity

iamp = .15
ipch  = cpsmidinn(p4) 
asig scantable iamp, ipch, 1, 2, 3, 4, 5
asig dcblock asig
asig   butlp  asig, 5000                              ; lowpass filter
outs asig, asig;

endin

</CsInstruments>
<CsScore>
s
i1	0	20	50  128
i1	10	10	70  .
i1	15	3	40  .
s
i1	0	20	50  4096    ; f-tables now bigger tables
i1	10	10	70  .       ; sounds different 
i1	15	3	40  .
s
i1	0	20	50  1000    ; still big tables
i1	10	10	70  .       ; but non-power of 2
i1	15	3	40  .
s
i1	0	20	50  20      ; small tables
i1	10	10	70  .       ; & non-power of 2
i1	15	3	40  .
e
</CsScore>
</CsoundSynthesizer>
