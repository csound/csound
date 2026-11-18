<CsTest>
description = "Rebinding an Opcode object to a constant its body writes reads that constant, unless init had the same one"

[expect]
exit = 0
output = ["rebind to constant checks done"]
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

opcode W(val:i):k
  val = 5
  kOut = val
  xout kOut
endop

opcode Set5(value:i):void
  value = 5
endop

opcode Read(value:i):k
  kOut = value
  xout kOut
endop

; The same as W, through nested UDOs only.
opcode Nested(val:i):k
  Set5(val)
  kOut = Read(val)
  xout kOut
endop

; W with a local ksmps is called by copy, not by reference: it copies its
; input at init, and keeps what init wrote, as a classic UDO does.
opcode WCopy(val:i):k
  setksmps 16
  val = 5
  kOut = val
  xout kOut
endop

; The same as W, with an output a caller can also give as the input.
opcode Both(val:i):(i,k)
  val = 5
  first:i = 1
  kOut = val
  xout first, kOut
endop

instr 1
  kCycle timeinstk

  ; Bound at init to a variable, which the body writes, then at performance
  ; to a constant: the body reads the constant, as it reads a variable it is
  ; rebound to.
  objects:Opcode[] = create(opcoderef("W"), 1)
  iV = 1
  kArray init objects[0], iV
  kArray perf objects[0], 4
  if iV != 5 then
    prints "FAILED: init did not write the variable by reference\n"
    exitnow -1
  endif

  single:Opcode = create(opcoderef("W"))
  iS = 1
  kSingle init single, iS
  kSingle perf single, 7

  nested:Opcode[] = create(opcoderef("Nested"), 1)
  iN = 1
  kNested init nested[0], iN
  kNested perf nested[0], 9

  ; The variable is the object's output too.
  both:Opcode[] = create(opcoderef("Both"), 1)
  iB = 1
  iB, kBoth init both[0], iB
  iB, kBoth perf both[0], 11

  ; A constant at init, a variable, then the constant again.
  back:Opcode[] = create(opcoderef("W"), 1)
  iW = 8
  kBack init back[0], 3
  if kCycle == 1 then
    kBack perf back[0], iW
  else
    kBack perf back[0], 3
  endif

  ; One constant at init, another at performance.
  other:Opcode[] = create(opcoderef("W"), 1)
  kOther init other[0], 54
  kOther perf other[0], 56

  ; The same constant at init and at performance keeps what init wrote, as a
  ; plain call does.
  kept:Opcode[] = create(opcoderef("W"), 1)
  kKept init kept[0], 6
  kKept perf kept[0], 6

  copied:Opcode[] = create(opcoderef("WCopy"), 1)
  iC = 1
  kCopied init copied[0], iC
  kCopied perf copied[0], 13

  if kArray != 4 then
    printks "FAILED: an array element rebound to 4 read %g\n", 0, kArray
    exitnowk -1
  endif
  if kSingle != 7 then
    printks "FAILED: a single object rebound to 7 read %g\n", 0, kSingle
    exitnowk -1
  endif
  if kNested != 9 then
    printks "FAILED: a body of nested UDOs rebound to 9 read %g\n", 0, kNested
    exitnowk -1
  endif
  if kBoth != 11 then
    printks "FAILED: rebound to 11 from the object's output variable, read %g\n", 0, kBoth
    exitnowk -1
  endif
  if kCycle == 1 && kBack != 8 then
    printks "FAILED: rebound to a variable holding 8, read %g\n", 0, kBack
    exitnowk -1
  endif
  if kCycle > 1 && kBack != 3 then
    printks "FAILED: rebound from a variable back to 3, read %g\n", 0, kBack
    exitnowk -1
  endif
  if kOther != 56 then
    printks "FAILED: rebound from 54 to 56, read %g\n", 0, kOther
    exitnowk -1
  endif
  if kKept != 5 then
    printks "FAILED: rebound to the constant init had, read %g\n", 0, kKept
    exitnowk -1
  endif
  if kCopied != 5 then
    printks "FAILED: a UDO called by copy, rebound to 13, read %g\n", 0, kCopied
    exitnowk -1
  endif

  if kCycle == 4 then
    printks "rebind to constant checks done\n", 0
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 .25
e
</CsScore>
</CsoundSynthesizer>
