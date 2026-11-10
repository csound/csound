<CsTest>
description = "An unterminated block comment in a skipped <CsOptions> is a missing end tag"
args = ["-nd", "-+ignore_csopts=1"]

[expect]
exit = "nonzero"
stderr = ["Missing end tag </CsOptions>"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
--omacro:FROM_CSOPTIONS=1 /* never closed
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
