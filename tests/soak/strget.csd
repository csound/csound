<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o strget.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

strset 1, "fox.wav"
strset 2, "beats.wav"

instr 1

Sfile strget p4
asig  soundin Sfile
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 2.7 1
i 1 + 2   2
e
</CsScore>
</CsoundSynthesizer>
