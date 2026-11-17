<CsTest>
description = "an ifdef at the end of an include resumes the parent buffer"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
#define ACTIVE #1#
#include "include_ifdef_at_eof.orc"
#endif
instr 1
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
