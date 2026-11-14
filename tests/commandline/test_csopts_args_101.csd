<CsTest>
description = "One option more than the limit stops Csound"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["More than 100 arguments"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d -d --omacro:FROM_CSOPTIONS=1
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
