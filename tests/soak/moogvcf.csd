<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o moogvcf.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; iscale does not need to be set here because already 0dbfs = 1  
aout vco .3, 220, 1	; Use a nice sawtooth waveform.
kfco line 200, p3, 2000	; filter-cutoff frequency from .2 to 2 KHz
krez init p4
asig moogvcf aout, kfco, krez
     outs asig, asig

endin
</CsInstruments>
<CsScore>
;a sine wave
f 1 0 16384 10 1

i 1 0 3 .1
i 1 + 3 .7
i 1 + 3 .95
e
</CsScore>
</CsoundSynthesizer>
