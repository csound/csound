<CsTest>
description = "Zak mixing, modulation and partial blocks preserve active samples"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
zakinit 4, 4
gkDone init 0
instr 1
  ; Keep the historical inclusive highest location and fractional truncation.
  ziw 2, 4.9
  ziwm 3, 4, 1
  iValue zir 4
  if iValue != 5 then
    exitnow -1
  endif
  zkw 2, 4.9
  zkwm 3, 4, 1
  kValue zkr 4
  kAdd zkmod 3, 4.9
  kMultiply zkmod 3, -4.9
  kBypass zkmod 3, -.9
  if kValue != 5 || kAdd != 8 || kMultiply != 15 || kBypass != 3 then
    exitnowk -1
  endif
  zkcl 4, 4
  kZero zkr 4
  if kZero != 0 then
    exitnowk -1
  endif
  aDrive = .25
  zaw aDrive, 4.9
  zawm aDrive, 4, 1
  aRead zar 4
  aGain zarg 4, 2
  aAdd zamod aDrive, 4.9
  aMultiply zamod aDrive, -4.9
  aBypass zamod aDrive, -.9
  kError max_k abs(aRead-2*aDrive)+abs(aGain-4*aDrive)+abs(aAdd-3*aDrive)+abs(aMultiply-2*aDrive*aDrive)+abs(aBypass-aDrive), 1, 1
  if !(kError < .000001) then
    printks "Zak audio error %g\n", 0, kError
    exitnowk -1
  endif
  zacl 4
  aZero zar 4
  kZeroError max_k abs(aZero), 1, 1
  if !(kZeroError < .000001) then
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkDone += 1
    kFirst = 0
  endif
endin
instr 99
  if i(gkDone) != 8 then
    prints "Zak checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0.000000000000 0.031250000000
i 1 0.065429687500 0.015625000000
i 1 0.147460937500 0.000976562500
i 1 0.187500000000 0.004882812500
i 1 0.252929687500 0.046875000000
i 1 0.375000000000 0.078125000000
i 1 0.502929687500 0.004882812500
i 1 0.584960937500 0.000976562500
i 99 1 .001
e
</CsScore>
</CsoundSynthesizer>
