<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o adds.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
; add unipolar square to oscil
kamp = p4
kcps = 1
itype = 3

klfo	lfo kamp, kcps, itype
printk2	klfo
asig	oscil 0.7, 440+klfo, 1
	outs asig, asig

endin


</CsInstruments>
<CsScore>
;sine wave.
f 1 0 32768 10 1

i 1 0 2 1	;adds 1 Hz to frequency
i 1 + 2 10	;adds 10 Hz to frequency
i 1 + 2 220	;adds 220 Hz to frequency

e

</CsScore>
</CsoundSynthesizer>
