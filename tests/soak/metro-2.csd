<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o metro-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kpch    random   1,20		;produce values at k-rate
ktrig   metro    10		;trigger 10 times per second
kval	samphold kpch, ktrig 	;change value whenever ktrig = 1 
asig	buzz	 1, 220, kval, 1;harmonics
        outs     asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 4096 10 1	; sine

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
