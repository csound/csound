<CsTest>
description = "A skipped <CsOptions> is not held to the limit on arguments"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = 0
output = ["CsOptions ignored"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d
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
