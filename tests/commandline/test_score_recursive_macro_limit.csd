<CsTest>
description = "recursive score macros stop at the nesting limit"

[expect]
exit = "nonzero"
stderr = ["macros/include nested too deep"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
instr 1
endin
</CsInstruments>
<CsScore>
#define RECURSE #$RECURSE.#
$RECURSE.
</CsScore>
</CsoundSynthesizer>
