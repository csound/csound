<CsTest>
description = "Csound objects preserve audio with different input/output channels and block sizes"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
0dbfs = 1

instr 1
  child:Csound = create()
  iResult = setoption(child, "-n")
  SCode sprintf {{
    sr = 1024
    ksmps = %d
    nchnls_i = %d
    nchnls = %d
    0dbfs = 1
    instr 1
      aInput inch 1
      outch 1, aInput
    endin
    schedule 1, 0, 1
  }}, p4, p5, p6
  iResult = compilestr(child, SCode)
  iResult = start(child)
  if iResult != 0 then
    exitnow -1
  endif
  aInput = .25
  outch child, 1, aInput
  if p5 == 2 then
    aOther = -.5
    outch child, 2, aOther
  endif
  kResult = perf(child)
  aOutput = inch(child, 1)
  kFrame init 0
  kN = 0
  while kN < ksmps do
    kExpected = (kFrame+kN < p4 ? 0 : .25)
    kSample vaget kN, aOutput
    if kSample != kExpected then
      printks "Csound audio mismatch at %g: got %g, expected %g\n", 0, kFrame+kN, kSample, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kFrame += ksmps
  delete child
endin
</CsInstruments>
<CsScore>
i 1 0 .5 16 1 2
i 1 .5 .5 16 2 1
i 1 1 .5 64 1 2
i 1 1.5 .5 64 2 1
i 1 2 .5 128 1 2
i 1 2.5 .5 128 2 1
e
</CsScore>
</CsoundSynthesizer>
