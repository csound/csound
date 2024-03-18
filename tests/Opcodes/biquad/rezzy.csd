<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o rezzy.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
0dbfs  = 1
nchnls = 2

instr 1

asaw vco2 .3, 110	;sawtooth
kcf  line 1760, p3, 220	;vary cut-off frequency from 220 to 1280 Hz
kres = p4		;vary resonance too
ares rezzy asaw, kcf, kres
asig balance ares, asaw
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 4 10
i 1 + 4 30
i 1 + 4 120	;lots of resonance
e
</CsScore>
</CsoundSynthesizer>
