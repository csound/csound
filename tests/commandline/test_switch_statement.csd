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

</CsInstruments>
<CsScore>
i 1 0 0 1
i 2 0 0
i 3 0 0
i 4 0 0
i 5 0 0.1
</CsScore>
</CsoundSynthesizer>
