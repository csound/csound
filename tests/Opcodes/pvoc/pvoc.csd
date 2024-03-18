<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  --limiter=.95 ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o pvoc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

gilen  filelen "fox.wav"	    ; get length of impulse soundfile

; analyze sound file and output result to pvoc-ex file
ires system_i 1,{{ pvanal fox.wav fox1.pvx }}          ; default settings

instr 1 ; untreated signal
asig    diskin2   "fox.wav", 1
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
outs    asig*.8, asig*.8
endin

instr 2

prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
ktime line 0, p3, gilen     ; timepointer over the entire sample
asig  pvoc ktime, p4, "fox1.pvx", 1 
      outs asig*.6, asig*.6
endin

</CsInstruments>
<CsScore>

i1 0 2.76      ; original sample

i2  5 2.76  1  ; no change so original sample is played
i2  10 2    1  ; played faster

i2  15 10   1  ; slowed down, same pitch &
i2  25 10   1.2; slowed down and higher pitch

e
</CsScore>
</CsoundSynthesizer>
