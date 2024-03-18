<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
 -M0 -Q1 ;;;midi in and midi out
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps line 3, p3, .1	
klf  lfo 1, kcps, 3	;use a unipolar square to trigger
ktr  trigger klf, 1, 1	;from 3 times to .1 time per sec.
     midion2 1, 60, 100, ktr

endin
</CsInstruments>
<CsScore>

i 1 0 20
e
</CsScore>
</CsoundSynthesizer>
