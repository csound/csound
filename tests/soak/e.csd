<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o e.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

ga1 init 0

instr 1

aenv expseg .01, p3*0.1, 1, p3*0.9, 0.01
ga1  poscil3 .5*aenv, cpspch(p4), 1
     outs ga1,ga1
endin

instr 99
 
aL, aR  reverbsc ga1, ga1, 0.85, 12000, sr, 0.5, 1
outs aL,aR

ga1 = 0

endin
</CsInstruments>
<CsScore>
f 1 0 128 10 1		;sine wave

i 1 1 0.1 8.00
i 1 2 0.1 8.02
i 1 3 0.1 8.04
i 1 4 0.1 9.06

i 99 0 6		;remains active for 6 seconds

e10
</CsScore>
</CsoundSynthesizer>

