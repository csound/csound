<CsTest>
description = "recursive orchestra macros stop at the nesting limit"

[expect]
exit = "nonzero"
stderr = ["macros/include nested too deep"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
#define RECURSE #$RECURSE.#
$RECURSE.
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
