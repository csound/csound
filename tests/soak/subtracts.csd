<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac    ;;;realtime audio I/O
; For Non-realtime ouput leave only the line below:
; -o -.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idur = p3
iatt = p4
idec = p5
isus = p3-iatt-idec				;calculate sustain time from subtracting attack and decay
printf_i "sutain time= note duration - attack - decay --> %.1f-%.1f-%.1f = %.1f\n", 1, idur, iatt, idec, isus

kenv expseg 0.01, iatt, 1, isus, 1, idec, 0.01	;envelope
asig poscil 1*kenv, 200, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 4096 10 1	;sine wave
;      attack decay
i 1 0 3 .1     .2
i 1 4 3 .5    1.5
i 1 8 5  4     .5

e
</CsScore>
</CsoundSynthesizer>

