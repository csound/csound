<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  --limiter=.99 ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o cvanal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

gilen  filelen "rv_stereo.wav"	    ; get length of impulse soundfile

; analyze sound file and output result to 3 convolve files
ires1 system_i 1,{{ cvanal rv_stereo.wav rv_stereo1.con }}                  ; default settings
ires2 system_i 1,{{ cvanal -d.15 rv_stereo.wav rv_stereo2.con }}            ; use only first portion
ires3 system_i 1,{{ cvanal -b.25 -d.001 rv_stereo.wav rv_stereo3.con }}    ; take very short portion from the end


instr 1 ; untreated signal
asig    diskin2   "drumsMlp.wav", 1
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
outs    asig, asig
endin

instr 2

prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
prints "--**used duration of impulse file = %5.3f seconds (total =%5.2f)**--\\n", p5, gilen

adry    diskin2 "drumsMlp.wav"                 ; input (dry) audio
awet1, awet2 convolve adry*.8, p4           ; stereo convolved (wet) audio
awet1  *=  p6                               ; scale amplitude of impulse sound, Left
awet2  *=  p6                               ; & Right channel
adrydel delay   adry, p5                    ; delay dry signal to align it with convolved signal
outs    (adrydel+awet1)*.8,(adrydel+awet2)*.8 ; mix wet & dry signals 
endin

</CsInstruments>
<CsScore>

i1 1 2          ; untreated signal

i2 4 3  "rv_stereo1.con"    0.39    .01	; use total impulse
i2 8 3  "rv_stereo2.con"    0.15    .01	; first portion of impulse
i2 12 3 "rv_stereo3.con"    0.001    1	; very short portion of impulse, starting from 0.25 sec, & scale amp
e
</CsScore>
</CsoundSynthesizer>
