<CsTest>
description = "A block comment after a quoted value still opens and hides its tags"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
--omacro:FROM_CSOPTIONS="a b" /* a comment
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
