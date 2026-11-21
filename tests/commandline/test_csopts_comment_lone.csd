<CsTest>
description = "A lone block comment opener at the start of a line is a comment"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken"]
output_excludes = ["Invalid arguments"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
/*
</CsOptions>
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
