<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dispfft.wav -W ;;; for file output any platform
</CsOptions>;be sure to NOT have -d in the CsOptions...
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

kcps = 110
ifn  = 1

knh	line p4, p3, p5
asig	buzz 1, kcps, knh, ifn
	outs asig, asig

dispfft	asig, .1, 2048, 0, 1

endin
</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 3 20 20
i 1 + 3 3 3
i 1 + 3 150 1
e

</CsScore>
</CsoundSynthesizer>
