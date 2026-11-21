<CsTest>
description = "Tags in a block comment do not end a skipped <CsOptions>"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = 0
output = ["CsOptions ignored"]
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
