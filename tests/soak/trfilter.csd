<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o trfilter.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifn ftgen 2, 0, -22050, 5, 1, 1000, 1, 4000, 0.000001, 17050, 0.000001 ; low-pass filter curve of 22050 points	

instr 1

kam  line 1, p3, p4
ain  diskin2 "beats.wav", 1, 0, 1
fs1,fsi2 pvsifd	ain, 2048, 512, 1		; ifd analysis
fst  partials fs1, fsi2, .003, 1, 3, 500	; partial tracking
fscl trfilter fst, kam, gifn			; filtering using function table 2
aout tradsyn fscl, 1, 1, 500, 1			; resynthesis 
     outs aout, aout

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 4 1
i 1 5 4 0	;reduce filter effect
e
</CsScore>
</CsoundSynthesizer>