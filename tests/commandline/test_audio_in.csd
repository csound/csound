<CsoundSynthesizer>
-+rtaudio=dummy
<CsInstruments>
sr=44100
ksmps=1
nchnls=1
nchnls_i=1

// parser test
// make sure the 'in' token
// isn't conflicting with for-in
instr 1
  ain1 in
  out ain1*0
endin

</CsInstruments>
<CsScore>
i1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
