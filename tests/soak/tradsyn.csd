<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tradsyn.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ipch = p4
ain  diskin2 "beats.wav", 1
fs1,fsi2 pvsifd ain,2048,512,1		; ifd analysis
fst  partials fs1,fsi2,.003,1,3,500	; partial tracking
aout tradsyn fst, 1, ipch, 500, 1	; resynthesis
     outs aout, aout

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 2 1.5	;up a 5th
i 1 + 2  .5	;octave down
e
</CsScore>
</CsoundSynthesizer>
