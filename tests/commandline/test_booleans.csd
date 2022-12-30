<CsoundSynthesizer>
<CsInstruments>

sr=44100
ksmps=1
nchnls=1

instr 1
  bi = 9 == 10 ;; false
  if bi then
    prints "fail\n"
  else
    prints "pass\n"
  endif
endin

instr 2
  ix = 0
  bi = ix == 1
  until bi do
    prints "pass\n" ;; should print only once
    ix += 1
    bi = ix == 1
  od
endin

</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
e
</CsScore>
</CsoundSynthesizer>
