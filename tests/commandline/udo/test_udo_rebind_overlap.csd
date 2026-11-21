<CsTest>
description = "Rebinding UDO outputs preserves newly overlapping global sources"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
gkA init 3
gkB init 8
opcode Pair(unused:k):(k,k)
 xout gkA, gkB
endop
opcode ClassicPair, kk, k
 kUnused xin
 xout gkA, gkB
endop
instr 1
 objects:Opcode[] = create(opcoderef("Pair"), 1)
 classic:Opcode[] = create(opcoderef("ClassicPair"), 1)
 kInit1, kInit2 init objects[0], 0
 kClassicInit1, kClassicInit2 init classic[0], 0
 gkA = 3
 gkB = 8
 gkB, gkA perf objects[0], 0
 if gkA != 8 || gkB != 3 then
  printks "Wrong global swap after rebinding\n", 0
  exitnowk -1
 endif
 ; Removing the overlap must not leave stale snapshot pointers behind.
 kFirst, kSecond perf objects[0], 0
 if kFirst != 8 || kSecond != 3 then
  exitnowk -1
 endif
 gkA = 3
 gkB = 8
 gkB, gkA perf classic[0], 0
 if gkA != 8 || gkB != 3 then
  printks "Wrong classic global swap after rebinding\n", 0
  exitnowk -1
 endif
 delete(objects)
 delete(classic)
endin
</CsInstruments>
<CsScore>
i1 0 .002
i1 .01 .002
</CsScore>
</CsoundSynthesizer>
