<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvswarp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kscal = p4
asig  soundin "fox.wav"			; get the signal in
fsig  pvsanal asig, 1024, 256, 1024, 1	; analyse it
ftps  pvswarp fsig, kscal, 0		; warp it
atps  pvsynth ftps			; synthesise it                      
      outs atps, atps

endin
</CsInstruments>
<CsScore>

i 1 0 3 1
i 1 + 3 1.5
i 1 + 3 3
i 1 + 3 .25
e
</CsScore>
</CsoundSynthesizer>
