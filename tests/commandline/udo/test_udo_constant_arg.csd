<CsTest>
description = "A numeric constant passed to a new-style UDO keeps its value whatever the UDO assigns to its argument"

[expect]
exit = 0
output = ["constant argument checks done"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

opcode Fail(Swhat:S):void
  prints "FAILED: %s\n", Swhat
  exitnow -1
endop

opcode SetI(val:i):void
  val = 5
  if val != 5 then
    Fail("the assignment did not reach the argument")
  endif
endop

opcode SetIReadK(val:i):k
  ; What init wrote lasts: an i-rate argument is not refreshed at k-rate.
  val = 5
  kOut = val
  xout kOut
endop

opcode SetK(val:k):void
  val = 7
  if val != 7 then
    printks "FAILED: the k-rate assignment did not reach the argument\n", 0
    exitnowk -1
  endif
endop

opcode CountK(val:k):k
  ; Each k-cycle starts from what the call gives, as in a classic UDO.
  val += 1
  kOut = val
  xout kOut
endop

opcode Inner(val:i):void
  val = 9
endop

opcode Outer(val:i):void
  Inner(val)
endop

opcode ReadK(val:k):k
  kOut = val * 2
  xout kOut
endop

instr 1
  ; Compare with what the cell held before the call, in variables of their
  ; own: a constant compared with itself would hide its being overwritten.
  iBefore = 1
  SetI(1)
  iAfter = 1
  if iBefore != iAfter then
    Fail("an i-rate write reached the constant")
  endif

  ; A nested UDO given the argument can write it too.
  iBefore = 3
  Outer(3)
  iAfter = 3
  if iBefore != iAfter then
    Fail("a nested write reached the constant")
  endif

  ; An i-rate argument keeps what init wrote through performance.
  kWritten = SetIReadK(4)
  iAfter = 4
  if iAfter != 4.0 then
    Fail("the write of an i-rate argument read at k-rate reached the constant")
  endif
  if kWritten != 5 then
    printks "FAILED: what init wrote to an i-rate argument did not last\n", 0
    exitnowk -1
  endif

  ; A variable is still written by reference.
  iVar = 8
  SetI(iVar)
  if iVar != 5 then
    Fail("a variable was not written by reference")
  endif

  kCycle timeinstk
  kBefore = 2
  SetK(2)
  kAfter = 2
  if kBefore != kAfter then
    printks "FAILED: a k-rate write reached the constant\n", 0
    exitnowk -1
  endif

  kCount = CountK(6)
  kExpected = 7
  if kCycle > 2 && kCount != kExpected then
    printks "FAILED: a written k-rate argument does not start each cycle from the call\n", 0
    exitnowk -1
  endif

  kRead = ReadK(2.5)
  if kRead != 5 then
    printks "FAILED: a constant read by the body was not 2.5\n", 0
    exitnowk -1
  endif

  if kCycle == 8 then
    printks "constant argument checks done\n", 0
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 .25
e
</CsScore>
</CsoundSynthesizer>
