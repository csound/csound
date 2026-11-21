<CsTest>
description = "Tags in a block comment after an option do not end <CsOptions>"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
--omacro:FROM_CSOPTIONS=1 /* a comment
</CsOptions>
</CsoundSynthesizer>
*/
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
