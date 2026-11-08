<CsTest>
expect.exit=0
description="Syntax-only imports do not run module initializers"
expect.output_excludes=["MODULE_SIDE_EFFECT","ROOT_EXECUTED"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --syntax-check-only
</CsOptions>
<CsInstruments>
sr=44100
ksmps=32
nchnls=2
0dbfs=1
import "syntax_effect.orc"
iCount chnget "module_init_count"
prints "ROOT_EXECUTED\n"
instr 1
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
