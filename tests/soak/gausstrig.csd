<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kdev line 0, p3, 0.9
seed 20120125
aimp gausstrig 0.5, 10, kdev
aenv filter2 aimp, 1, 1, 0.993, 0.993
anoi fractalnoise 0.2, 1.7
al   = anoi*aenv
ar   delay al, 0.02
outs al, ar

endin
</CsInstruments>
<CsScore>
i1 0 10
e
</CsScore>
</CsoundSynthesizer>
