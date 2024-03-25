<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvstrace.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kn = p4
asig  diskin2	"drumsMlp.wav", 1
fsig  pvsanal   asig, 1024, 256, 1024, 1; analyse it
ftps  pvstrace   fsig, kn		; keep kn bins
atps  pvsynth   ftps			; synthesise it
      outs      atps, atps

endin
</CsInstruments>
<CsScore>
	
i1 0 2  5
i1 + 2  10
i1 + 2  20
i1 + 2  40
i1 + 2  80
i1 + 2  160
e
</CsScore>
</CsoundSynthesizer>
