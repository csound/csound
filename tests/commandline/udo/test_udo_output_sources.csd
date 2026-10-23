<CsTest>
description = "UDO outputs preserve global, system, and repeated local values"
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
0dbfs=1
giSource init 23
gkSource init 0

opcode GlobalValue():i
  xout giSource
endop
opcode SystemValues():(i,i,i,i)
  xout sr, kr, ksmps, 0dbfs
endop
opcode LocalPair():(i,i)
  value:i init 5
  xout value, value
endop
opcode ControlGlobal():k
  gkSource += 2
  xout gkSource
endop
opcode ControlPair(value:k):(k,k)
  result:k = value + 1
  xout result, result
endop
opcode AudioPair(value:k):(a,a)
  signal:a = value
  xout signal, signal
endop
opcode ArrayPair():(i[],i[])
  values:i[] fillarray 4, 7
  xout values, values
endop
opcode StringPair():(S,S)
  value:S init "tone"
  xout value, value
endop

instr 1
  iGlobal = GlobalValue()
  iSr, iKr, iKsmps, iScale = SystemValues()
  iFirst, iSecond = LocalPair()
  iArray1[], iArray2[] = ArrayPair()
  SFirst, SSecond = StringPair()
  if iGlobal != 23 || giSource != 23 || iSr != sr || iKr != kr || iKsmps != ksmps || iScale != 0dbfs || iFirst != 5 || iSecond != 5 then
    prints "Wrong init output: %g %g %g %g %g %g %g\n", iGlobal, iSr, iKr, iKsmps, iScale, iFirst, iSecond
    exitnow -1
  endif
  if iArray1[0] != 4 || iArray2[1] != 7 || strcmp(SFirst, "tone") != 0 || strcmp(SSecond, "tone") != 0 then
    prints "Wrong array or string output\n"
    exitnow -1
  endif
  gkSource += 1
  kGlobal = ControlGlobal()
  kFirst, kSecond = ControlPair(gkSource)
  aFirst, aSecond = AudioPair(gkSource)
  kAudio1 downsamp aFirst
  kAudio2 downsamp aSecond
  if kGlobal != gkSource || kFirst != gkSource + 1 || kSecond != kFirst then
    printks "Wrong control output: %g %g %g %g\n", 0, kGlobal, gkSource, kFirst, kSecond
    exitnowk -1
  endif
  if kAudio1 != gkSource || kAudio2 != gkSource then
    printks "Wrong audio output: %g %g\n", 0, kAudio1, kAudio2
    exitnowk -1
  endif
  ; The next call must replace caller changes with the current source value.
  kGlobal += 100
  kSecond += 100
endin
</CsInstruments>
<CsScore>
i1 0 .01
i1 .02 .01
</CsScore>
</CsoundSynthesizer>
