<CsTest>
description = "for loops honor variable type prefixes and run at the resolved rate"

[expect]
exit = 0
output = ["implicit k-rate loop passed", "for-loop type precedence passed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; Issue #3444: an i-array must not force k0 into an init-time loop.
; Check every value on two control cycles to verify performance-time execution.
instr 1
  kxs[] init 20
  kcycle init 0
  kcycle += 1
  for k0 in [0 ... 10] do
    kxs[k0] = k0 * 2 + kcycle
  od
  kcheck = 0
  while kcheck < 20 do
    kexpected = kcheck <= 10 ? kcheck * 2 + kcycle : 0
    if kxs[kcheck] != kexpected then
      printks "implicit k-rate loop failed at %d: got %g, expected %g\n", 0, kcheck, kxs[kcheck], kexpected
      exitnowk -1
    endif
    kcheck += 1
  od
  if kcycle == 2 then
    printks "implicit k-rate loop passed\n", 0
    turnoff
  endif
endin

instr 2
  ; Unprefixed names still inherit the array element type.
  isum init 0
  for value in [1, 2, 3] do
    isum += value
  od

  ; An explicit type takes precedence over a conflicting name prefix.
  iexplicit init 0
  for kvalue:i in [1, 2, 3] do
    iexplicit += kvalue
  od

  ; A previously declared variable retains its type despite its name prefix.
  kexisting:i init 0
  iexisting init 0
  for kexisting in [1, 2, 3] do
    iexisting += kexisting
  od
  if isum != 6 || iexplicit != 6 || iexisting != 6 then
    prints "init-time loop type precedence failed\n"
    exitnow -1
  endif

  kcycle init 0
  kcycle += 1
  ksum = 0
  ; Explicit k-type overrides the i-prefix, and the index follows the loop rate.
  for ivalue:k, index in [1, 2, 3] do
    ksum += ivalue + index + kcycle
  od
  values:k[] fillarray 1, 2, 3
  kinferred = 0
  for element in values do
    kinferred += element + kcycle
  od
  if ksum != 9 + 3 * kcycle || kinferred != 6 + 3 * kcycle then
    printks "performance-time loop type precedence failed\n", 0
    exitnowk -1
  endif
  if kcycle == 2 then
    printks "for-loop type precedence passed\n", 0
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
</CsScore>
</CsoundSynthesizer>
