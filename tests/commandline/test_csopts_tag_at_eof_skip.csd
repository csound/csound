<CsTest>
description = "The same end tag ends a skipped <CsOptions>"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = 0
output = ["CsOptions ignored"]
</CsTest>
<CsoundSynthesizer>
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
<CsOptions>
--omacro:FROM_CSOPTIONS=1
</CsOptions>