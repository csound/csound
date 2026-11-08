<CsTest>
description = "Rebinding UDO outputs refreshes repeated output sources"
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
opcode Pair(value:k):(k,k)
 result:k = value + 1
 xout result, result
endop
opcode AudioPair(value:k):(a,a)
 result:a = value
 xout result, result
endop
opcode ArrayPair(value:k):(k[],k[])
 result:k[] init 2
 result[0] = value
 result[1] = value + 1
 xout result, result
endop
opcode Outer(value:k):(k,k)
 first:k, second:k = Pair(value)
 xout first, second
endop
instr 1
 objects:Opcode[] = create(opcoderef("Pair"), 1)
 audioObjects:Opcode[] = create(opcoderef("AudioPair"), 1)
 arrayObjects:Opcode[] = create(opcoderef("ArrayPair"), 1)
 nested:Opcode[] = create(opcoderef("Outer"), 1)
 kInit1, kInit2 init objects[0], 2
 aInit1, aInit2 init audioObjects[0], 2
 kInitArray1[], kInitArray2[] init arrayObjects[0], 2
 kNestedInit1, kNestedInit2 init nested[0], 2
 kArray1[] init 2
 kArray2[] init 2
 kValue init 3
 kValue += 1
 kOut1, kOut2 perf objects[0], kValue
 if kOut1 != kValue + 1 || kOut2 != kValue + 1 then
  printks "Wrong repeated output after rebinding\n", 0
  exitnowk -1
 endif
 ; Rebind the same object again, then return to the first pair next cycle.
 kNext1, kNext2 perf objects[0], kValue + 10
 if kNext1 != kValue + 11 || kNext2 != kValue + 11 then
  exitnowk -1
 endif
 aOut1, aOut2 perf audioObjects[0], kValue
 kAudio1 downsamp aOut1
 kAudio2 downsamp aOut2
 kArray1, kArray2 perf arrayObjects[0], kValue
 kNested1, kNested2 perf nested[0], kValue
 if kAudio1 != kValue || kAudio2 != kValue || kArray1[0] != kValue || kArray2[1] != kValue + 1 || kNested1 != kValue + 1 || kNested2 != kValue + 1 then
  printks "Wrong audio, array, or nested output after rebinding\n", 0
  exitnowk -1
 endif
 delete(objects)
 delete(audioObjects)
 delete(arrayObjects)
 delete(nested)
endin
</CsInstruments>
<CsScore>
i1 0 .01
i1 .02 .01
</CsScore>
</CsoundSynthesizer>
