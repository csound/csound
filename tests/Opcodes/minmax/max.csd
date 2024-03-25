<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o max.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

k1   oscili 1, 10.0, 1			;combine 3 sinusses
k2   oscili 1, 1.0, 1			;at different rates
k3   oscili 1, 3.0, 1
kmax max   k1, k2, k3
kmax = kmax*250				;scale kmax
printk2 kmax				;check the values

aout vco2 .5, 220, 6			;sawtooth
asig moogvcf2 aout, 600+kmax, .5 	;change filter around 600 Hz		
     outs asig, asig

endin

</CsInstruments>
<CsScore>
f1 0 32768 10 1

i1 0 5
e
</CsScore>
</CsoundSynthesizer>
