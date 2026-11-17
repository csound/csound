<CsTest>
description = "A skipped <CsOptions> treats those same markers the same way"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = 0
output = ["CsOptions ignored"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
--omacro:FROM_CSOPTIONS=1
--omacro:VALUE=a/*b
--omacro:QUOTED="c /* d"
-d "--omacro:ALSO=e /* f"
"quoted first /* word"
--omacro:ESCAPED=g\ /*h
; /* a line comment
# /* a line comment
// /* a line comment
-d ; /* a line comment
-d # /* a line comment
-d // /* a line comment
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
