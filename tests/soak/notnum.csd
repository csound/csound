<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -M0 -+rtmidi=virtual ;;;realtime audio out with virtual MIDI in
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

massign 1, 1	; set MIDI channel 1 to play instr 1

instr   1

iNum notnum
     print iNum
; Convert MIDI note number to Hz
iHz  = (440.0*exp(log(2.0)*((iNum)-69.0)/12.0))
aosc oscil 0.6, iHz, 1
     outs  aosc, aosc

endin
</CsInstruments>
<CsScore>
f 1 0  16384 10 1	;sine wave

f 0 60			;play 60 seconds

e
</CsScore>
</CsoundSynthesizer>
