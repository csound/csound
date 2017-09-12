<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o trcross.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ain1 diskin2 "beats.wav", 1, 0, 1
ain2 diskin2 "fox.wav", 1

imode = p4
fs1,fsi2 pvsifd ain1, 2048, 512, 1		; ifd analysis
fst      partials fs1, fsi2, .01, 1, 3, 500	; partial tracking

fs11,fsi12 pvsifd ain2, 2048, 512, 1		; ifd analysis (second input)
fst1     partials fs11, fsi12, .01, 1, 3, 500	; partial tracking (second input

fcr  trcross fst, fst1, 1.05, 1, imode		; cross-synthesis (mode 0 and mode 1)
aout tradsyn fcr, 1, 1, 500, 1			; resynthesis of tracks
     outs aout*3, aout*3

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 3 0
i 1 5 3 1

e
</CsScore>
</CsoundSynthesizer>