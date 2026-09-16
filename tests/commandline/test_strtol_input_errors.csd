<CsTest>
description = "strtol rejects values outside its integer range and malformed input"
[expect]
exit = "nonzero"
stderr = ["integer out of range", "invalid format", "empty string"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  SValue strget p4
  iValue strtol SValue
endin
instr 2
  iValue strtol p4
endin
instr 3
  strset 100, "2147483648"
  iValue strtol 100
endin
instr 4
  iValue strtol 1e30
endin
</CsInstruments>
<CsScore>
i1 0 .01 "2147483648"
i1 0 .01 "-2147483649"
i1 0 .01 "020000000000"
i1 0 .01 "-020000000001"
i1 0 .01 "0x80000000"
i1 0 .01 "-0x80000001"
i1 0 .01 "999999999999999999999999999999"
i1 0 .01 "+"
i1 0 .01 "0x"
i1 0 .01 "08"
i1 0 .01 "12x"
i1 0 .01 "12 "
i1 0 .01 ""
i2 0 .01 "2147483648"
i3 0 .01
i4 0 .01
</CsScore>
</CsoundSynthesizer>
