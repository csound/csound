<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o else.wav -W ;;; for file output any platform
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
	kenv adsr 0.01, 0.95, .7, .5
else
	kenv linseg 0, p3 * .5, 1, p3 * .5, 0
endif

aout vco2    .8, ipch, 10
aout moogvcf aout, ipch + (kenv * 6 * ipch) , .5

aout = aout * kenv
    outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 2 8.00 0
i 1 3 2 8.00 1

e
</CsScore>
</CsoundSynthesizer>