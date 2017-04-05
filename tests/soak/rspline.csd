<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o jspline.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

krangeMin init p4
krangeMax init p5
kcpsmin init 2
kcpsmax init 3

ksp  rspline krangeMin, krangeMax, kcpsmin, kcpsmax
aout pluck 1, 200+ksp, 1000, 0, 1
aout dcblock aout	;remove DC
     outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0  10  2  5	;a bit jitter
i 1 8  10 10 20	;some more
i 1 16 10 20 50	;lots more
e
</CsScore>
</CsoundSynthesizer>
