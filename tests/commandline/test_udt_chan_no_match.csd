<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Expected failure test.
; Writes one UDT layout to a generic channel, then attempts to read it
; back as a different UDT layout. The init-time type mismatch is the
; assertion, so test.py expects a nonzero exit status.

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
