<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o delayk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;example shows "delayk" for fm index and 
;a second "delayk" for panning
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

gisin ftgen 0, 0, 2^10, 10, 1

instr 1

kenv1 transeg 0, .02, 0, 1, 3.98, -6, 0 ;envelope
kenv2 delayk kenv1, 2			;delayed by two seconds
kindx expon 5, p3, 1			;fm index decreasing over p3
asig  foscili .6, 400, 1, 11/4, kindx, gisin
kpan1 linseg 0, 4, 1			;panning for first sound
kpan2 linseg 1, 4, 0			;panning for second sound ...
kpan2 delayk kpan2, 2			;delayed by two seconds
a1 = asig * kenv1
a2 = asig * kenv2

aL1,aR1 pan2 a1, kpan1
aL2,aR2 pan2 a2, kpan2
        outs aL1+aL2, aR1+aR2

endin
</CsInstruments>
<CsScore>

i 1 0 6
e
</CsScore>
</CsoundSynthesizer>