<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o follow2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

asig soundin "beats.wav"
     outs asig, asig
endin

instr 2 ;using follow2

as soundin "beats.wav"
af follow2 as, p4, p5
ar rand 44100	;noise
; "beats.wav" provides amplitude for noise
asig balance ar, af
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 2
i 2 2 2 0.001 0.01 ;quick attack & deacy
i 2 5 2  0.1   0.5  ;slow attack & deacy	

e
</CsScore>
</CsoundSynthesizer>
