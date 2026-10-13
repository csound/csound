<CsTest>
description = "Complex exp and log give the same values when their input is reused as the output"

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
gkCycles init 0

opcode Check, 0, kkkkS
  kReal, kImag, kExpectedReal, kExpectedImag, SCase xin
  if !(abs(kReal - kExpectedReal) < 0.00001 && abs(kImag - kExpectedImag) < 0.00001) then
    printks "%s: expected (%g, %g), got (%g, %g)\n", 0, SCase, kExpectedReal, kExpectedImag, kReal, kImag
    exitnowk -1
  endif
endop

instr 1
  ; Both storage forms represent 0.3 + 0.7i.
  kOriginal:Complex = complex(p4 == 0 ? 0.3 : sqrt(0.58), p4 == 0 ? 0.7 : taninv2(0.7, 0.3), p4)
  SCase sprintf "polar=%d", p4

  ; Calculate the expected components directly from the complex identities.
  iExpReal = exp(0.3) * cos(0.7)
  iExpImag = exp(0.3) * sin(0.7)
  iLogReal = log(sqrt(0.58))
  iLogImag = taninv2(0.7, 0.3)

  kSeparateExp:Complex = exp(kOriginal)
  kSeparateLog:Complex = log(kOriginal)
  ; Reset from the unchanged source each cycle, then overwrite the input.
  kReusedExp:Complex = kOriginal
  kReusedExp = exp(kReusedExp)
  kReusedLog:Complex = kOriginal
  kReusedLog = log(kReusedLog)

  iReal = real(kReusedExp)
  iImag = imag(kReusedExp)
  if abs(iReal - iExpReal) > 0.00001 || abs(iImag - iExpImag) > 0.00001 then
    prints "Reusing complex exp input changes its init-time result\n"
    exitnow -1
  endif
  iReal = real(kReusedLog)
  iImag = imag(kReusedLog)
  if abs(iReal - iLogReal) > 0.00001 || abs(iImag - iLogImag) > 0.00001 then
    prints "Reusing complex log input changes its init-time result\n"
    exitnow -1
  endif

  Check real(kSeparateExp), imag(kSeparateExp), iExpReal, iExpImag, strcat(SCase, " exp, separate output")
  Check real(kReusedExp), imag(kReusedExp), iExpReal, iExpImag, strcat(SCase, " exp, reused input")
  Check real(kSeparateLog), imag(kSeparateLog), iLogReal, iLogImag, strcat(SCase, " log, separate output")
  Check real(kReusedLog), imag(kReusedLog), iLogReal, iLogImag, strcat(SCase, " log, reused input")

  gkCycles += 1
  kCycle timeinstk
  if kCycle == 3 then
    turnoff
  endif
endin

instr 2
  ; The array overloads must also support mixed storage forms in place.
  kOriginal:Complex[] = [complex(0.3, 0.7), polar(complex(0.3, 0.7))]
  iExpReal = exp(0.3) * cos(0.7)
  iExpImag = exp(0.3) * sin(0.7)
  iLogReal = log(sqrt(0.58))
  iLogImag = taninv2(0.7, 0.3)

  kSeparateExp:Complex[] = exp(kOriginal)
  kSeparateLog:Complex[] = log(kOriginal)
  kReusedExp:Complex[] = kOriginal
  kReusedExp = exp(kReusedExp)
  kReusedLog:Complex[] = kOriginal
  kReusedLog = log(kReusedLog)

  kIndex = 0
  while kIndex < 2 do
    Check real(kSeparateExp[kIndex]), imag(kSeparateExp[kIndex]), iExpReal, iExpImag, "array exp, separate output"
    Check real(kReusedExp[kIndex]), imag(kReusedExp[kIndex]), iExpReal, iExpImag, "array exp, reused input"
    Check real(kSeparateLog[kIndex]), imag(kSeparateLog[kIndex]), iLogReal, iLogImag, "array log, separate output"
    Check real(kReusedLog[kIndex]), imag(kReusedLog[kIndex]), iLogReal, iLogImag, "array log, reused input"
    kIndex += 1
  od

  gkCycles += 1
  kCycle timeinstk
  if kCycle == 3 then
    turnoff
  endif
endin

instr 99
  if i(gkCycles) != 9 then
    prints "Complex exp/log checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 1
i 2 0 .01
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
