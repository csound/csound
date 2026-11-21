<CsTest>
description = "An unterminated block comment at the start of a line is a missing end tag"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["Missing end tag </CsOptions>"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
/* never closed
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
