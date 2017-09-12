<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac  -Q1   ;;;realtime audio out with MIDI out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	; change attack time of external synth

initc7 1, 6, 0		; set controller 6 to 0
nrpn 1, 99, 1		; set MSB
nrpn 1, 98, 99		; set LSB
katt ctrl7 1, 6, 1, 127	; DataEntMSB
idur = 2
noteondur2 1, 60, 100, idur ; play note on synth

endin
</CsInstruments>
<CsScore>

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
