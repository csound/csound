<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o dnoise.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2022

; analyze sound file and output result, and be verbose 
ires1 system_i 1,{{ dnoise -V -W        -i MathewsNoise.wav -o outfile01.wav MathewsN.wav }}                  ; default settings (1024 bandoass filters)
ires2 system_i 1,{{ dnoise -V -W -N8    -i MathewsNoise.wav -o outfile02.wav MathewsN.wav }}                  ; LoFi, only 8 bandpass filters
ires3 system_i 1,{{ dnoise -V -W -N4096 -i MathewsNoise.wav -o outfile03.wav MathewsN.wav }}                  ; 4096 bandpass filters used

instr 1 ; untreated signal
asig    diskin2   "MathewsN.wav", 1                                         ; sample = 17.6 secs
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
outs    asig, asig
endin

instr 2

prints  "\n---***YOU NOW HEAR THE DENOISED FILE WITH DEFAULT SETTINGS:***---\n"
adnoise diskin2 p4, 1                 
outs    adnoise, adnoise
endin

instr 3

prints  "\n---***YOU NOW HEAR THE LoFi FILE:***---\n"
adnoise diskin2 p4, 1                 
outs    adnoise, adnoise
endin

instr 4

prints  "\n---***YOU NOW HEAR THE DENOISED FILE WITH MANY BANDS, CREATING SOME ECHOES:***---\n"
adnoise diskin2 p4, 1                 
outs    adnoise, adnoise
endin
</CsInstruments>
<CsScore>

i1 1 17.6                   ; untreated signal

i2 20 17.6 "outfile01.wav"  ; default settings 
i3 40 17.6 "outfile02.wav"  ; LoFi
i4 60 17.6 "outfile03.wav"  ; echo-y
e
</CsScore>
</CsoundSynthesizer>
