<CsTest>
description = "an undef at the end of an include resumes the parent buffer"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
#define TO_REMOVE #1#
#include "include_undef_at_eof.orc"
#ifdef TO_REMOVE
this is not valid orchestra code
#end
instr 1
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
