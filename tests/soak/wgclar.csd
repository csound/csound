<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o wgclar.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kfreq = 330
kstiff = -0.3
iatt = 0.1
idetk = 0.1
kngain init p4		;vary breath
kvibf = 5.735
kvamp = 0.1

asig wgclar .9, kfreq, kstiff, iatt, idetk, kngain, kvibf, kvamp, 1
     outs asig, asig
      
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave

i 1 0 2 0.2
i 1 + 2 0.5		;more breath
e

</CsScore>
</CsoundSynthesizer>
