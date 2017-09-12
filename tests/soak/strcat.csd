<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o strcat.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

Sname =  "beats"
Sname strcat  Sname, ".wav"
asig  soundin Sname
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 2
e
</CsScore>
</CsoundSynthesizer>
