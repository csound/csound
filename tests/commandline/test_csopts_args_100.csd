<CsTest>
description = "One hundred options on a line, the limit, are all taken"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d --omacro:FROM_CSOPTIONS=1
</CsOptions>
<CsInstruments>

instr 1
#ifdef FROM_CSOPTIONS
  prints "CsOptions taken\n"
#else
  prints "CsOptions ignored\n"
#end
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
