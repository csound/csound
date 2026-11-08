<CsTest>
description = "Overload lookup skips empty input and output signatures"
[expect]
exit = 0
stderr = ["Total assertions: 3", "Failed: 0"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --run-unit-tests
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode SignatureInput(value:i):i
  xout value + 1
endop
opcode SignatureInput():i
  xout 77
endop
opcode SignatureOutput(value:i):i
  xout value + 2
endop
opcode SignatureOutput(value:i):void
endop

instr 1
  iInput SignatureInput 41
  iOutput SignatureOutput 40
  assert(iInput == 42)
  assert(iOutput == 42)
  assert(SignatureInput() == 77)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
