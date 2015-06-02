<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o moog.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kfreq  = cpspch(p4)
kfiltq = p5
kfiltrate = 0.0002
kvibf  = 5
kvamp  = .01
;low volume is needed
asig moog .15, kfreq, kfiltq, kfiltrate, kvibf, kvamp, 1, 2, 3
     outs asig, asig

endin
</CsInstruments>
<CsScore>

f 1 0 8192 1 "mandpluk.aiff" 0 0 0
f 2 0 256 1 "impuls20.aiff" 0 0 0
f 3 0 256 10 1	; sine

i 1 0 3 6.00 .1
i 1 + 3 6.05 .89
i 1 + 3 6.09 .50
e
</CsScore>
</CsoundSynthesizer>
