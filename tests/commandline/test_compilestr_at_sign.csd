<CsTest>
description = "compilestr preserves at signs in source and compiled string literals"
[expect]
exit = 0
stderr = ["compiled@literal", "dynamic global=7"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iResult compilestr {{
dynamicValue@global:i init 7
instr 2
  prints "compiled@literal\\n"
  printf_i "dynamic global=%g\\n", 1, dynamicValue
endin
}}
  if iResult != 0 then
    exitnow -1
  endif
  event_i "i", 2, .02, .01
endin
</CsInstruments>
<CsScore>
i 1 0 .01
f 0 .05
e
</CsScore>
</CsoundSynthesizer>
