<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvread.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; analyze "fox.wav" with PVANAL first
ibin  = p4
ktime line 0, p3, 2.8
kfreq, kamp pvread ktime, "fox.pvx", ibin	;read data from 7th analysis bin.
asig  poscil kamp, kfreq, 1 			;function 1 is a stored sine
      outs asig*5, asig*5			;compensate loss of volume

endin
</CsInstruments>
<CsScore>
;sine wave
f1 0 4096 10 1

i 1 0 6 7
i 1 + 6 15
i 1 + 2 25
e
</CsScore>
</CsoundSynthesizer>
