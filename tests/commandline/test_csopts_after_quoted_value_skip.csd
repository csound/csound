<CsTest>
description = "A block comment after the options that follow a quoted value ending a word opens in a skipped <CsOptions> too"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = 0
output = ["CsOptions ignored"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
--omacro:QUOTED="a b" --omacro:FROM_CSOPTIONS=1 /* a comment
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
