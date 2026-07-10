<CsoundSynthesizer>
<CsInstruments>

sr=44100
ksmps=1
nchnls=1

instr 1
  bi = 9 == 10 ;; false
  if bi then
    exitnow(-1)
  else
    prints "pass\n"
  endif

  result:i = 9 != 10 && 1 == 1
  if result != 1 then
    prints "boolean-to-i conversion failed: expected 1, got %d\n", result
    exitnow(-1)
  endif
endin

instr 2
  ix = 0
  bi = ix == 1
  until bi do
    if ix > 0 then
       exitnow(-1)
    else
      prints "pass\n"
    endif
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
