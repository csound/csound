<CsTest>
description = "a call with a computed argument writes its output itself, as the same call with a constant does"

[expect]
exit = 0
output = ["kept state for 3 periods", "exported 7", "held the frame"]
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
gkGlobal = k(1 + 2)
; chnexport finds its output among the globals by name
giExported = chnexport("exported", 1 + 2)

opcode Counted():k
  count:k = k(1 + 2)
  count += 1
  xout(count)
endop

instr 1
  kCycle init 0
  kCycle += 1
  values:i[] = fillarray(2, 3)

  ; each gives 4, 5, 6: k() of an i value writes at init, and the first period adds 1
  sum:k = k(1 + 2)
  sum += 1
  kNamed = k(1 + 2)
  kNamed += 1
  called:k = k(random:i(3, 3))
  called += 1
  field:k = k(p4 + 1)
  field += 1
  element:k = k(values[1])
  element += 1
  gkGlobal += 1
  counted:k = Counted()

  ; peak compares with its output, so a reset of that output holds
  kLevel = kCycle == 1 ? 1 : 0.5
  loudest:k = peak(kLevel * 1)

  kExpected = 3 + kCycle
  kLoudest = kCycle == 1 ? 1 : 0.5
  if sum != kExpected || kNamed != kExpected || called != kExpected ||
     field != kExpected || element != kExpected || gkGlobal != kExpected ||
     counted != kExpected || loudest != kLoudest then
    printks "period %d: sum %g named %g called %g field %g element %g global %g udo %g peak %g\n", 0, kCycle, sum, kNamed, called, field, element, gkGlobal, counted, loudest
    exitnowk(-1)
  endif
  loudest = 0
  if kCycle == 3 then
    printks "kept state for %d periods\n", 0, kCycle
    turnoff
  endif
endin

instr 2
  chnset(7, "exported")
endin

instr 3
  if giExported != 7 then
    prints "exported channel missed the variable: %g\n", giExported
    exitnow(-1)
  endif
  prints "exported %g\n", giExported
endin

; pvsinit writes once: what a later line writes stays, and is not put back to zero
instr 4
  kCycle init 0
  kCycle += 1
  source:a = oscili:a(0.3, 440)
  analysed:f = pvsanal(source, 1024, 256, 1024, 1)
  constant:f = pvsinit(1024)
  computed:f = pvsinit(1000 + 24)
  if kCycle > 100 && kCycle < 200 then
    constant = pvsgain(analysed, 1)
    computed = pvsgain(analysed, 1)
  endif
  kConstant, kFreq1 pvsbin constant, 10
  kComputed, kFreq2 pvsbin computed, 10
  if kConstant != kComputed then
    printks "period %d: constant %g computed %g\n", 0, kCycle, kConstant, kComputed
    exitnowk(-1)
  endif

  ; an fsig given to the call that writes it, with a constant and computed
  scaled:f = pvsanal(source, 1024, 256, 1024, 1)
  fMoved = pvsanal(source, 1024, 256, 1024, 1)
  scaled = pvscale(scaled, 1.5)
  fMoved = pvscale(fMoved, kCycle / kCycle * 1.5)
  kScaled, kFreq3 pvsbin scaled, 15
  kMoved, kFreq4 pvsbin fMoved, 15
  if kScaled != kMoved then
    printks "period %d: scaled %g moved %g\n", 0, kCycle, kScaled, kMoved
    exitnowk(-1)
  endif
  if kCycle == 300 then
    if kConstant <= 0 || kMoved <= 0 then
      printks "no frame: held %g moved %g\n", 0, kConstant, kMoved
      exitnowk(-1)
    endif
    printks "held the frame\n", 0
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1 2
i 2 0 0
i 3 0.01 0
i 4 0.02 1
e
</CsScore>
</CsoundSynthesizer>
