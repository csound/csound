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

#include "http://codemist.co.uk/jpff/test.in"

instr 2

Sfile strfromurl "http://codemist.co.uk/jpff/test.in"
  prints Sfile
endin
</CsInstruments>
<CsScore>

i 1 0 1
i 2 1 1
e
</CsScore>
</CsoundSynthesizer>
