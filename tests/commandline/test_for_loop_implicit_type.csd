<CsTest>
description = "for loops honor variable type prefixes and run at the resolved rate"

[expect]
exit = 0
output = ["implicit k-rate loop passed", "for-loop type precedence passed", "for-loop prefix edge cases passed"]
stderr = ["ignoring type annotation 'i' for existing loop variable 'existing' (type 'k')"]
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

instr 3
  ; The index inherits the loop rate, even with a conflicting name prefix.
  isum init 0
  for value, kndx in [1, 2, 3] do
    if strcmp(typeof(kndx), "i") != 0 then
      prints "index prefix changed its type\n"
      exitnow -1
    endif
    isum += kndx
  od
  ; Descriptive names with standard prefixes can explicitly request i-type.
  for freq:i in [1, 2, 3] do
    isum += freq
  od
  for wave:i in [1, 2, 3] do
    isum += wave
  od
  controls:k[] fillarray 1, 2, 3
  for iElement in controls do
    isum += iElement
  od
  if isum != 21 then
    prints "explicit prefix override failed\n"
    exitnow -1
  endif

  kcycle init 0
  kcycle += 1
  ksum = 0
  ; Strip the g prefix when resolving the element's implicit type.
  for gkvalue in [1, 2, 3] do
    ksum += gkvalue + kcycle
  od
  ; An explicit index annotation selects performance-time iteration.
  kindexsum = 0
  for element, ndx:k in [1, 2, 3] do
    kindexsum += element + ndx + kcycle
  od
  ; An existing variable wins over a conflicting annotation, with a warning.
  existing:k init 0
  kexistingsum = 0
  for existing:i in [1, 2, 3] do
    kexistingsum += existing + kcycle
  od
  if ksum != 6 + 3 * kcycle || kindexsum != 9 + 3 * kcycle || kexistingsum != 6 + 3 * kcycle then
    printks "for-loop prefix edge cases failed\n", 0
    exitnowk -1
  endif
  if kcycle == 2 then
    printks "for-loop prefix edge cases passed\n", 0
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
i 3 0 0.01
</CsScore>
</CsoundSynthesizer>
