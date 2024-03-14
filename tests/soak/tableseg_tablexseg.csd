<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0   ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o tableseg_tablexseg.wav -W ;;; for file output any platform
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

if p5 == 1 then        ; make a choice
        tablexseg 1, p3, 2	; morph from table 1 to table 2
        prints  "2 tables morph exponentially (tables 1 & 2)\n"
    else 
        tableseg 1, p3, 2	; morph from table 1 to table 2
        prints  "2 tables morph in a linear way (tables 1 & 2)\n"
endif

asig  vpvoc ktime, p4, "fox1.pvx"
      outs asig, asig
endin

</CsInstruments>
<CsScore>
f 1 0 512 9 .5 1 0
f 2 0 512 7 1 512 0

i1  0 2.76          ; original sample

i2  5 2.76  1    0  ; linear morph 
i2  8 2.76  1    1  ; exponential morph of filter

i2  10 10   1    0  ; linear morph
i2  10 10   1.7  0   
i2  20 10   1    1  ; exponential morph
i2  20 10   1.7  1  

e
</CsScore>
</CsoundSynthesizer>
