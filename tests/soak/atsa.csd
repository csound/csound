<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0 --limiter=.95 ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o atsa.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

gilen  filelen "fox.wav"	    ; get length of soundfile

; analyze sound file and output result to 3 ats files. ATSA wants a mono file!
ires1 system_i 1,{{ atsa fox.wav fox1.ats }}            ; default settings
ires2 system_i 1,{{ atsa -h.8 fox.wav fox2.ats }}       ; some smearing
ires3 system_i 1,{{ atsa -h.1 -c2 fox.wav fox3.ats }}   ; only 2 cycles

instr 1 ; untreated signal
asig    diskin2   "fox.wav", 1
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
prints "---*duration of soundfile is %f seconds*---\\n",gilen
outs    asig, asig
endin

instr 2
prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
ktime line 0, p3, gilen                         ; timepointer = equal to soundfile duration
asig ATSsinnoi ktime, 1, .4, 1, p4, 167         ; maximum # of partials = 167
outs asig, asig	    
endin

</CsInstruments>
<CsScore>

i1 2 2.76               ; original sample

i2 5 2.76  "fox1.ats"	; whole sentence
i2 10 2.76 "fox2.ats"	; whole sentence, but analyzed with different settings
i2 15 2.76 "fox3.ats"	; whole sentence, and again analyzed with different settings
e
</CsScore>
</CsoundSynthesizer>
