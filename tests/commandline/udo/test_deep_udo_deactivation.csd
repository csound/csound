<CsTest>
description = "deep UDO chains deactivate without exhausting the C stack"
args = ["-nd"]
application_args = []
stack_limit_kb = 512

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

opcode RetainedBranch(depth:i):k
  if (depth == 0) then
    result:k = timeinstk()
  else
    left:k = RetainedBranch(depth - 1)
    right:k = RetainedBranch(depth - 1)
    result:k = left + right
  endif
  xout result
endop

instr 1
  ; Leave room for Clang's sanitizer frames during compilation. Retain
  ; 4095 UDO frames with only 11 init calls so recursive teardown still
  ; exceeds the C stack limit.
  result:k = RetainedBranch(11)
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
