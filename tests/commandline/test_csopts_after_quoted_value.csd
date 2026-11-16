<CsTest>
description = "The options after a quoted value that ends a word are read, and a block comment after them opens"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken", "after the comment"]
output_excludes = ["unknown CSD tag"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
--omacro:QUOTED="a b" --omacro:FROM_CSOPTIONS=1 /* a comment
</CsOptions>
*/
--omacro:AFTER_COMMENT=1
</CsOptions>
<CsInstruments>

instr 1
#ifdef FROM_CSOPTIONS
  prints "CsOptions taken\n"
#else
  prints "CsOptions ignored\n"
#end
#ifdef AFTER_COMMENT
  prints "after the comment\n"
#end
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
