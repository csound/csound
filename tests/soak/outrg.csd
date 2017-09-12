<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outrg.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4        ;quad
0dbfs  = 1

instr 1

kleft init 1
asig  vco2 .5, 220			;sawtooth
idur = p3/(nchnls-1)
knext init idur
kpos init 0
krate init 1/idur
kbase init 0
ktime timeinsts
if ktime>=knext then
  kleft = kleft + 1
  knext = knext + idur
  kpos = 0
  kbase = ktime
else
  kpos = (ktime-kbase)/idur
endif
printks "speaker %d position %f\n", 0, kleft, kpos
a1,a2 pan2 asig, kpos
      outrg  kleft, a1, a2
kpos = kbase/idur
endin

</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>

