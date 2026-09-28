<CsTest>
description = "Numbered LPC slots and interpolated sources remain local to each instance"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode ChildSlots, 0, 0
  ; Use the same numbers with different data to expose any shared slots.
  lpslot 0
  kR0, kO0, kE0, kC0 lpread 0, "lpc_slots_2000hz.lpc"
  lpslot 1
  kR1, kO1, kE1, kC1 lpread 0, "lpc_slots_2000hz.lpc"
  lpslot 70
  lpinterp 0, 1, 0
endop

instr Interpolate
  kCycle init 0
  if kCycle == 100 then
    reinit SOURCES
  endif
SOURCES:
  ; Each file has a conjugate pole pair of radius .5 at 1000 or 2000 Hz.
  lpslot 0
  kR0, kO0, kE0, kC0 lpread 0, "lpc_slots_1000hz.lpc"
  lpslot 1
  kR1, kO1, kE1, kC1 lpread 0, "lpc_slots_2000hz.lpc"
  ChildSlots
  ; The destination may replace an input slot, as in the manual example.
  lpslot p5
  lpinterp 0, 1, p4
  rireturn

  if kCycle == 200 then
    reinit FILTERS
  endif
FILTERS:
  ; A consumer-only reinit must still see the interpolated source.
  kFrequency, kBandwidth lpform 1
  aOut lpreson a(.1)
  rireturn
  if kCycle == 80 || kCycle == 180 || kCycle == 280 then
    iFrequency = 1000 + 1000*p4
    iBandwidth = -log(.5)*sr/$M_PI
    ; The constant-input gain follows from this conjugate pole pair.
    iExpected = .1/(1 - cos(2*$M_PI*iFrequency/sr) + .25)
    if abs(kFrequency - iFrequency) > .01 || abs(kBandwidth - iBandwidth) > .01 \
        || abs(k(aOut) - iExpected) > .00001 then
      printks "LPC interpolation used the wrong numbered slots\n", 0
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kCycle += 1
endin

instr CheckCompletion
  if i(gkChecks) != 9 then
    prints "Not every LPC interpolation check completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Interpolate" 0 .04 0 1
i "Interpolate" .01 .04 .5 70
i "Interpolate" .07 .04 1 1
i "CheckCompletion" .13 0
e
</CsScore>
</CsoundSynthesizer>
