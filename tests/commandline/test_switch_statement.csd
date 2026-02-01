<CsoundSynthesizer>
<CsInstruments>

// accepts p-fields
instr 1
  switch p4
    case 1
      prints "pass\n"
    default
     prints "fail\n"
  endsw
endin

// accepts expression
instr 2
  switch 1 + 1
    case 2
      prints "pass\n"
    default
     prints "fail\n"
  endsw
endin

// accepts DRY multi match cases
instr 3
  switch 3
    case 0, 2
      prints "fail\n"
    case 1, 3
      prints "pass\n"
    default
     prints "fail\n"
  endsw
endin

// accepts expression in case
instr 4
  switch 3
    case 1 + 2
      prints "pass\n"
    default
     prints "fail\n"
  endsw
endin

// operates on performance rate
instr 5
  kl = line(0, p3 - 1/kr, 1)
  switch int(kl)
    case 1
      printks2 "pass %d\n", kl
  endsw
endin

// cases can be combined
instr 6
  iAssertCase = 0
  iAssertDefault = 0
  switch p4
    case 1
    case 2
      prints "1 or 2\n"
      iAssertCase = 1
    default
      prints "Default\n"
      iAssertDefault = 1
  endsw
  if iAssertCase == 0 && p4 < 3 then
    prints "assert error, expected 1 or 2\n"
    exitnow(-1)
  endif
  if iAssertDefault == 0 && p4 >= 3 then
    prints "assert error, default was expected but did not execute\n"
    exitnow(-1)
  endif
endin

// cases without statement are correctly ignored
instr 7
  iAssertDefault = 0
  switch p4
    case 1
    case 2
    default
      prints "Default, val: %d\n", p4
      iAssertDefault = 1
  endsw
  if iAssertDefault == 0 && p4 == 3 then
    prints "assert error, default not executed when expected\n"
    exitnow(-1)
  endif
  if iAssertDefault == 1 && p4 < 3 then
    prints "assert error, fell back to default when not expected\n"
    exitnow(-1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0 1
i 2 0 0
i 3 0 0
i 4 0 0
i 5 0 0.1
i 6 0 0 1
i 6 0 0 2
i 6 0 0 3
i 7 0 0 1
i 7 0 0 2
i 7 0 0 3
</CsScore>
</CsoundSynthesizer>
