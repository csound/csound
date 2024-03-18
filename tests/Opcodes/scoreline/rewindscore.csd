<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o rewindscore.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
 
instr 1

kenv expon  1, p3, 0.0001
aout poscil .5*kenv, cpspch(p4), 1
     outs   aout, aout
endin

instr 100

rewindscore

endin
</CsInstruments>
<CsScore>
f1 0 4096 10 1	;sine wave

i1 0 1 8.00
i1 + 1 8.03
i1 + 1 8.04
i1 + 1 8.07 
i1 + 1 8.09
i1 + 1 8.10
i1 + 1 8.09
i1 + 1 8.07
i1 + 1 8.03

i100 9 1	;rewind from 9th second
e
</CsScore>
</CsoundSynthesizer>

