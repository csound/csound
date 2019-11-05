<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1  ;expects MIDI note inputs on channel 1

iamp	ampmidi 1	; scale amplitude between 0 and 1
asig	oscil iamp, 220, 1
	print iamp
	outs  asig, asig

endin

</CsInstruments>
<CsScore>
;Dummy f-table for 1 minute
f 0 60
;sine wave.
f 1 0 16384 10 1

e

</CsScore>
</CsoundSynthesizer>
