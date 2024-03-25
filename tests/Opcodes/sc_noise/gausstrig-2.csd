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

kfrq0 oscil 8, 0.25, 1
ktrig metro 1
kfrq  samphold kfrq0+8, ktrig 
seed  20120125
aimp  gausstrig 0.5, kfrq, 0.5, 1
aenv  filter2 aimp, 1, 1, 0.993, 0.993
anoi  fractalnoise 0.2, 1.7
al    = anoi*aenv
ar    delay al, 0.02
outs  al, ar

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1
i1 0 16
e
</CsScore>
</CsoundSynthesizer>
