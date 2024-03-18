<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  --limiter=.95 ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o pvanal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

gilen  filelen "fox.wav"	    ; get length of impulse soundfile

; analyze sound file and output result to 3 pvoc-ex files
ires1 system_i 1,{{ pvanal fox.wav fox1.pvx }}          ; default settings
ires2 system_i 1,{{ pvanal -K -w1 fox.wav fox2.pvx }}   ; very low indow setting
ires3 system_i 1,{{ pvanal -n256 fox.wav fox3.pvx }}    ; different frame size

instr 1 ; untreated signal
asig    diskin2   "fox.wav", 1
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
outs    asig*.8, asig*.8
endin

instr 2

prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
ktime line 0, p3, gilen/2.4     ; slow down to have a good listen at what happens
asig  pvoc ktime, 1, p4, 1 
prints  "(playback is slowed down & limited to 'the quick brown fox')\n"
      outs asig*.8, asig*.8
endin

</CsInstruments>
<CsScore>

i1 0 2.76               ; original sample

i2  5 10  "fox1.pvx"    ; default but slowed down
i2 16 10  "fox2.pvx"    ; low window setting
i2 27 10  "fox3.pvx"    ; smearing
e
</CsScore>
</CsoundSynthesizer>
