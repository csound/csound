<CsTest>
description = "String p-fields survive numeric copies, arrays and UDOs"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode CopyNumber, i, i
  iValue xin
  xout iValue
endop

opcode DecodeNumber, S, i
  iValue xin
  SValue strget iValue
  xout SValue
endop

opcode NestedDecode, S, i
  setksmps 1
  iValue xin
  SValue DecodeNumber iValue
  xout SValue
endop

instr 1
  SDirect strget p5
  iCopy = p5
  SCopy strget iCopy
  iArray[] passign 4, 5
  SArray strget iArray[1]
  iIndexed pindex 5
  SIndexed strget iIndexed
  iAssigned passign 5, 5
  SAssigned strget iAssigned
  iUdo CopyNumber p5
  SUdo strget iUdo
  SNested NestedDecode p5
  prints "direct=%s copy=%s array=%s pindex=%s passign=%s udo=%s nested=%s\n", SDirect, SCopy, SArray, SIndexed, SAssigned, SUdo, SNested
  if strcmp(SDirect, "beta") != 0 || strcmp(SCopy, "beta") != 0 || strcmp(SArray, "beta") != 0 || strcmp(SIndexed, "beta") != 0 || strcmp(SAssigned, "beta") != 0 || strcmp(SUdo, "beta") != 0 || strcmp(SNested, "beta") != 0 then
    exitnow -1
  endif
  strset 17, "overwritten"
  p5 = 17
  SAfter strget p5
  if strcmp(SAfter, "overwritten") != 0 then
    prints "String metadata overrode a numeric p-field write\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 "alpha" "beta"
e
</CsScore>
</CsoundSynthesizer>
