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
  ; This retains 2047 performance-rate UDO frames while initialization
  ; itself is only 10 calls deep. Teardown must not use the C call stack.
  result:k = RetainedBranch(10)
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
