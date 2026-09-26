<CsTest>
description = "tvconv clears stored input and pending output on reinitialization"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckReset
 kCycle init 0
 kWrite init 1
 kCycle += 1
 if kCycle == 8 then
  ; Fill both buffers first, then reset with both inputs frozen.
  ; A fresh convolver must stay silent, even though the inputs are nonzero.
  kWrite = 0
  reinit CONVOLVE
 endif
 aInput = .25
 aCoefficients = .125
CONVOLVE:
 aOutput tvconv aInput, aCoefficients, kWrite, kWrite, p4, 8
 rireturn
 if kCycle == 7 then
  kBefore downsamp aOutput
  if !(kBefore > .1) then
   printks "tvconv reset setup did not fill the buffers\n", 0
   exitnowk(-1)
  endif
 endif
 if kCycle >= 8 then
  kSample = 0
  while kSample < ksmps do
   kActual vaget kSample, aOutput
   if kActual != 0 then
    printks "tvconv retained state: partition=%g cycle=%g sample=%g output=%g\n", 0, p4, kCycle, kSample, kActual
    exitnowk(-1)
   endif
   kSample += 1
  od
 endif
 if kCycle == 16 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 3 then
  prints "tvconv reset checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckReset" 0 .04 1
i "CheckReset" 0 .04 2
i "CheckReset" 0 .04 8
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
