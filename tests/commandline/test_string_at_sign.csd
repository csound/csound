<CsTest>
description = "at signs in string literals do not act as global annotations"
[expect]
exit = 0
stderr = ["name@example.org|@|name@global|@@tail@", "value@position=42", "global=42"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
answer@global:i init 42
instr 1
  printf_i "%s|%s|%s|%s\n", 1, "name@example.org", "@", "name@global", "@@tail@"
  printf_i "value@position=%g\n", 1, answer
  printf_i "global=%g\n", 1, answer
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
