<CsTest>
description = "functional init with a rate and a computed value keeps state, like the same call with a constant"

[expect]
exit = 0
output = ["kept state for 3 periods"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; global code has no performance pass, so a copy from a hidden variable never runs
gkGlobal = init:k(1 + 2)

opcode Counted():k
  count:k = init:k(1 + 2)
  count += 1
  xout(count)
endop

instr 1
  kCycle init 0
  kCycle += 1

  ; each gives 4, 5, 6: state starts at 3 and the first period adds 1
  sum:k = init:k(1 + 2)
  sum += 1
  kNamed = init:k(1 + 2)
  kNamed += 1
  called:k = init:k(random:i(3, 3))
  called += 1
  field:k = init:k(p4 + 1)
  field += 1
  signal:a = init:a(1 + 2)
  signal += 1
  gkGlobal += 1
  counted:k = Counted()

  kExpected = 3 + kCycle
  kSignal = k(signal)
  if sum != kExpected || kNamed != kExpected || called != kExpected ||
     field != kExpected || kSignal != kExpected || gkGlobal != kExpected ||
     counted != kExpected then
    printks "period %d: sum %g named %g called %g field %g signal %g global %g udo %g\n", 0, kCycle, sum, kNamed, called, field, kSignal, gkGlobal, counted
    exitnowk(-1)
  endif
  if kCycle == 3 then
    printks "kept state for %d periods\n", 0, kCycle
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1 2
e
</CsScore>
</CsoundSynthesizer>
