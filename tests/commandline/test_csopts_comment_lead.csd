<CsTest>
description = "A block comment at the start of a line is a comment and hides its tags"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
/* a comment
</CsOptions>
</CsoundSynthesizer>
*/
--omacro:FROM_CSOPTIONS=1
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
