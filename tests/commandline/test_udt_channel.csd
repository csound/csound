<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

struct Test val1:k, val2:i

instr 1
var:Test init 0, 2
var.val1 = 3
chnset var, "test"
var2:Test chnget "test"

if var2.val2 != var.val2 then
  prints "i-values do not match"
  exitnow(-1)
else
  print var2.val2
endif

if var2.val1 != var.val1 then
  event "i", 2, 0, 0
  printks "k-values do not match", 1
else 
  printk2 var2.val1
endif
endin

instr 2
exitnow(-1)
endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
