<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o portk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Stefano Cucchi 2020

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

kFreq randomh 400, 1300, 4, 2, 550 ; random frequency to oscillators.

khtim linseg 0.001, p3*0.3, 0.001, p3*0.7, 0.125 ; portamento-function: start with NO portamento - then portamento.
kPort portk kFreq, khtim ; pitch with portamento.

asigL oscili 0.4, kFreq, 1 ; channel left - NO portamento.
asigR oscili 0.4, kPort, 2 ; channel right - PORTAMENTO.

outch 1, asigL   ; channel left
outch 2, asigR   ; channel right

endin

</CsInstruments>
<CsScore>

f 1 0 4096 10 1 0 1 0 1 0 1 0 1
f 2 0 4096 10 1 1 0 1 0 1 0 1 0 1

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
