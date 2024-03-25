<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -m0d --midi-key-cps=4  -F midiChords.mid
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
</CsOptions>
<CsInstruments>

; by Menno Knevel - 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; midiChords.mid can be found in examples folder

instr 1

ivel veloc 0, 1			;re-scale velocity to 0 - 1
print ivel				;print velocity
asig vco2 .1*ivel, p4   ;
     outs asig, asig
       
endin
</CsInstruments>
<CsScore>

i1 0 35     ;midi file = 35 seconds

e
</CsScore>
</CsoundSynthesizer>
