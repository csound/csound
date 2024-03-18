<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0 --limiter=.95 ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o hetro.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

gilen  filelen "fox.wav"	    ; get length of soundfile

; analyze sound file and output result to 3 hetro files
ires1 system_i 1,{{ hetro fox.wav fox1.het }}           ; default settings
ires2 system_i 1,{{ hetro -f250 fox.wav fox2.het }}     ; high starting frequency
ires3 system_i 1,{{ hetro -f100 -h180 fox.wav fox3.het }}; up to 18kHz!

instr 1 ; untreated signal
asig    diskin2   "fox.wav", 1
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
prints "---*duration of soundfile is %f seconds*---\\n",gilen
outs    asig, asig
endin

instr 2
prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
asig      adsyn     1, 1, 1, p4 
outs asig, asig	    
endin

</CsInstruments>
<CsScore>

i1 0 2.76               ; original sample

i2 5 2.76  "fox1.het"	; whole sentence
i2 10 2.76 "fox2.het"	; whole sentence, but analyzed with different settings
i2 15 2.76 "fox3.het"	; whole sentence, and again analyzed with different settings
e
</CsScore>
</CsoundSynthesizer>
