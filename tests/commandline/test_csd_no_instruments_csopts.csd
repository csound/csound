<CsTest>
description = "A CSD with <CsOptions> and no <CsInstruments> stops before it performs"
args = ["-nd"]
# Without the score, a run that performs would not end (scoreless operation).

[expect]
exit = "nonzero"
stderr = ["has no <CsInstruments>"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-d
</CsOptions>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
