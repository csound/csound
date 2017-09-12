<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sinsyn.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kmxtr init p4
ain   diskin2 "fox.wav", 1
fs1,fsi2 pvsifd ain, 2048, 512,1	; ifd analysis
fst   partials fs1, fsi2, .03, 1, 3, 500 ; partial tracking
aout  sinsyn fst, .5, kmxtr, 1		; scale amplitude down
      outs aout, aout

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 2.7 15	;filtering effect by using low number of tracks
i 1 + 2.7 500	;maximum number of tracks
e
</CsScore>
</CsoundSynthesizer>
