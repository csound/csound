<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode GrowNumeric, k[], k
  kLast xin
  kOutput[] init 1
  kOutput genarray 0, kLast
  xout kOutput
endop

opcode ForwardNumeric, k[], k[]
  kInput[] xin
  kCycle timeinstk
  xout kInput
endop

opcode GrowStrings, S[], k
  kLast xin
  SOutput[] init 1
  SOutput[kLast] = sprintfk("item-%d", kLast)
  xout SOutput
endop

opcode ForwardStrings, S[], S[]
  SInput[] xin
  kCycle timeinstk
  xout SInput
endop

instr 1
  kLast init 1
  kNumeric[] GrowNumeric kLast
  kCopied[] ForwardNumeric kNumeric
  SStrings[] GrowStrings kLast
  SCopied[] ForwardStrings SStrings
  kNumericSize lenarray kCopied
  kStringSize lenarray SCopied
  kMatches strcmpk SCopied[kLast], sprintfk("item-%d", kLast)
  if (kNumericSize != kLast + 1 || kCopied[kLast] != kLast ||
      kStringSize != kLast + 1 || kMatches != 0) then
    printks "built-in array UDO copy lost resized data\n", 0
    exitnowk(-1)
  endif
  kLast += 1
  if (kLast > 4) then
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
