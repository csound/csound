<CsTest>
description = "pvsgain keeps downstream processing current after a source restart"

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

instr CheckGain
  kCycle init 0
  kCycle += 1
  kData[] init 66
  ; Give every bin a distinct magnitude and frequency, including DC/Nyquist.
  kLevel = (kCycle < 10 ? 1 : 3)
  kBin = 0
  while kBin < 33 do
    kData[2*kBin] = kLevel + kBin/64
    kData[2*kBin+1] = 128*kBin
    kBin += 1
  od
  if kCycle == 10 then
    reinit SOURCE
  endif
SOURCE:
  fSource pvsfromarray kData, p4
  rireturn

  ; Restarting only pvsgain must also publish a new output frame.
  kGain = (kCycle < 20 ? 2 : 4)
  if kCycle == 20 then
    reinit GAIN
  endif
GAIN:
  fLouder pvsgain fSource, kGain
  rireturn
  ; With both freeze flags off, this downstream opcode should copy every frame.
  fDownstream pvsfreeze fLouder, 0, 0
  kOutput[] init 66
  kFrame pvs2array kOutput, fDownstream
  kBin = 0
  while kBin < 33 do
    kExpected = kGain*(kLevel + kBin/64)
    if kOutput[2*kBin] != kExpected || kOutput[2*kBin+1] != 128*kBin then
      printks "hop %g cycle %g bin %g: amplitude %g (expected %g), frequency %g (expected %g)\n", 0, p4, kCycle, kBin, kOutput[2*kBin], kExpected, kOutput[2*kBin+1], 128*kBin
      exitnowk(-1)
    endif
    kBin += 1
  od
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 90 then
    prints "Not all pvsgain checks finished\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Each note runs 30 blocks. Also cover frames held across control blocks.
i "CheckGain" 0 .05859375 16
i "CheckGain" 0 .05859375 32
i "CheckGain" 0 .05859375 40
i "CheckCompletion" .08 .01
e
</CsScore>
</CsoundSynthesizer>
