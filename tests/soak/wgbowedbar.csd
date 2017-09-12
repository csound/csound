<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o wgbowedbar.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kp   = p6
asig wgbowedbar p4, cpspch(p5), 1, kp, 0.995
     outs asig, asig

endin
</CsInstruments>
<CsScore>
s
i1 0 .5 .5 7.00 .1	;short sound
i1 + .  .3 8.00 .1
i1 + .  .5 9.00 .1
s
i1 0 .5 .5 7.00  1	;longer sound
i1 + .  .3 8.00  1
i1 + .  .5 9.00  1
 
e
</CsScore>
</CsoundSynthesizer>

