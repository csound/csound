<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o moogvcf2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
aout diskin2 "beats.wav", 1, 0, 1
kfco line 100, p3, 10000		;filter-cutoff
krez init p4
asig moogvcf2 aout, kfco, krez
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 4 .1
i 1 + 4 .6
i 1 + 4 .9
e
</CsScore>
</CsoundSynthesizer>
