<CsTest>
description = "Named array signatures select the matching UDO overload"
[expect]
exit = 0
stderr = ["Total assertions: 2", "Failed: 0"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --run-unit-tests
</CsOptions>
<CsInstruments>
struct Voice value:i
struct Other value:i, extra:i
opcode ReadValue(items:Voice[]):i
  xout items[0].value
endop
opcode ReadValue(items:Other[]):i
  xout items[0].extra
endop
opcode PassVoice(items:Voice[]):Voice[]
  xout items
endop
instr 1
  voice:Voice init 42
  voices:Voice[] fillarray voice
  other:Other init 1, 17
  others:Other[] fillarray other
  copies:Voice[] = PassVoice(voices)
  assert(ReadValue(copies) == 42)
  assert(ReadValue(others) == 17)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
