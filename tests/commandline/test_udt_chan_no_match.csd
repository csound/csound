<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

struct Test1 var:i, var2:k
struct Test2 var1:i, var2:i

instr 1
 test1:Test1 init 1,2
 test2:Test2 init 1,2
 chnset test1, "Test1"
 test2 chnget "Test1"
endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
