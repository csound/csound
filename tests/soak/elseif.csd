<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o elseif.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

ipch = cpspch(p4)
ienv = p5


if (ienv == 0) then 	
	;ADSR
	kenv adsr 0.05, 0.05, .95, .05
elseif (ienv == 1) then 
	;Linear Triangular Envelope
	kenv linseg 0, p3 * .5, 1, p3 * .5, 0
elseif (ienv == 2) then 
	;Ramp Up
	kenv	linseg 0, p3 - .01, 1, .01, 0
endif

aout	vco2 	.8, ipch, 10
aout	moogvcf	aout, ipch + (kenv * 5 * ipch) , .5

aout = aout * kenv

outs aout, aout
endin
</CsInstruments>
<CsScore>

i 1 0 2 8.00 0
i 1 3 2 8.00 1
i 1 6 2 8.00 2
e
</CsScore>
</CsoundSynthesizer>