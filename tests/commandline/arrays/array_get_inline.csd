<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

ksmps = 32

opcode toStrArray, S[], S
  StringIn xin
  SArray[] fillarray StringIn
  xout SArray
endop

opcode toInitArray, i[], i
  iNumIn xin
  iNumArrayOut[] fillarray iNumIn
  xout iNumArrayOut
endop

opcode explicitReturn():i[]
  xout([1,2,3])
endop


instr 1
  String = toStrArray("Inline string-array get")[0]
  prints "%s\n", String
endin

instr 2
  itestVal = 123.456
  iresultVal = toInitArray(itestVal)[0]
  prints "%d\n", itestVal
endin

instr 3
  print(explicitReturn()[2])
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 + .
i 3 + .
</CsScore>
</CsoundSynthesizer>
Prints:
Inline string-array get
123.456
