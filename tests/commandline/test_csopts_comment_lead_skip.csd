<CsTest>
description = "A line-leading block comment hides its tags in a skipped <CsOptions>"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = 0
output = ["CsOptions ignored"]
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
