<CsTest>
description = "An end tag that is the last thing in the file, with no newline, ends <CsOptions>"
args = ["-nd"]

[expect]
exit = 0
output = ["CsOptions taken"]
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