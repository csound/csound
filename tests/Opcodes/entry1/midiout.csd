<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac -Ma -Q1 ;;;realtime audio out and midi out and midi in (all midi inputs)
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

midiout	192, 1, 21, 0	;program change to instr. 21
inum notnum
ivel veloc
midion 1, inum, ivel

endin
</CsInstruments>
<CsScore>

i 1 0 3  80 100		;play note for 3 seconds

e
</CsScore>
</CsoundSynthesizer>

