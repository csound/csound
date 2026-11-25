<CsTest>
description = "scale2 retains tiny coefficients and updates below one float ULP"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckFirstUpdate
  ; c1 is about 2^-26, too small for 1-c1 to differ from 1 in binary32.
  kOutput scale2 1, 0, 1, 0, 1, log(2)*pow(2,26)/kr
  if !(kOutput > 0 && abs(kOutput*pow(2,26)-1) < .00001) then
    printks "scale2 lost its small first update: %g\n", 0, kOutput
    exitnowk -1
  endif
  gkChecks += 1
endin

instr CheckRoundedUpdate
  kStep init 0
  iNext = 1+pow(2,-23)
  kTarget = (kStep < 40 ? 1 : iNext)
  kOutput scale2 kTarget, 0, 2, 0, 2, 1/kr
  if kStep == 40 then
    ; The first half-ULP update may round back to 1 in a float build.
    if !(kOutput >= 1 && kOutput <= iNext) then
      printks "scale2 first adjacent-value update is out of range: %.9f\n", 0, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  elseif kStep == 41 then
    if !(kOutput > 1 && kOutput <= iNext) then
      printks "scale2 did not retain its rounded update: %.9f\n", 0, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kStep += 1
endin

instr CheckCompletion
  if i(gkChecks) != 3 then
    prints "All slow-smoothing checks must run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckFirstUpdate" 0 [1/1024]
i "CheckRoundedUpdate" 0 [42/1024]
i "CheckCompletion" [44/1024] [1/1024]
e
</CsScore>
</CsoundSynthesizer>
