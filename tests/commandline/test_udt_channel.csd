<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Struct under test: one k-rate member and one i-rate member.
struct Test val1:k, val2:i

; Globals capture values read by later validator instruments.
gkTestkVal1 init -1
gkTestkVal2 init -1
gkTestiVal1 init -1
gkTestiVal2 init -1
gkTestkFromIVal1 init -1
gkTestkFromIVal2 init -1

; Writes three channels:
; 1) same-instrument generic chnset/chnget
; 2) i-time chnseti/chngeti across instruments
; 3) k-time chnsetk/chngetk across instruments
instr 1
same:Test init 0, 2
same.val1 = 3
chnset same, "test"

; Immediate same-instrument readback checks generic channel storage.
sameCheck:Test chnget "test"
if sameCheck.val2 != 2 then
  prints "same-instrument i-value mismatch\n"
  exitnow(-1)
endif

; i-time write preserves only the i-rate member by design.
itime:Test init 0, 11
itime.val1 = 6
chnseti itime, "testi"

; k-time write should preserve both members on later k-time reads.
ktime:Test init 0, 13
ktime.val1 = 8
chnsetk ktime, "testk"
endin

; Captures an i-time readback for deferred validation.
instr 2
itimeCheck:Test chngeti "testi"
gkTestiVal1 = itimeCheck.val1
gkTestiVal2 = itimeCheck.val2
turnoff
endin

; Captures a k-time readback for deferred validation.
instr 3
ktimeCheck:Test chngetk "testk"
gkTestkVal1 = ktimeCheck.val1
gkTestkVal2 = ktimeCheck.val2
endin

; Preloads a channel at i-time for a later k-time readback.
instr 4
preload:Test init 5, 7
chnseti preload, "testk_from_i"
endin

; Captures the k-time readback of an i-time write.
instr 5
fromI:Test chngetk "testk_from_i"
gkTestkFromIVal1 = fromI.val1
gkTestkFromIVal2 = fromI.val2
endin

; Validates chnsetk/chngetk across instruments.
instr 6
if timeinstk() > 0 then
  if abs(gkTestkVal1 - 8) > 0.000001 then
    printks "k-rate channel k-member mismatch: %f\n", 0, gkTestkVal1
    exitnowk(-1)
  endif
  if gkTestkVal2 != 13 then
    printks "k-rate channel i-member mismatch: %f\n", 0, gkTestkVal2
    exitnowk(-1)
  endif
  turnoff
endif
endin

; Validates chnseti/chngeti semantics across instruments.
instr 7
if timeinstk() > 0 then
  if abs(gkTestiVal1 - 0) > 0.000001 then
    printks "i-time channel k-member mismatch: %f\n", 0, gkTestiVal1
    exitnowk(-1)
  endif
  if gkTestiVal2 != 11 then
    printks "i-time channel value mismatch: %f\n", 0, gkTestiVal2
    exitnowk(-1)
  endif
  turnoff
endif
endin

; Validates that an i-time write is visible to a later k-time read.
instr 8
if timeinstk() > 0 then
  if abs(gkTestkFromIVal1 - 5) > 0.000001 then
    printks "i-to-k channel k-member mismatch: %f\n", 0, gkTestkFromIVal1
    exitnowk(-1)
  endif
  if gkTestkFromIVal2 != 7 then
    printks "i-to-k channel i-member mismatch: %f\n", 0, gkTestkFromIVal2
    exitnowk(-1)
  endif
  turnoff
endif
endin

</CsInstruments>

<CsScore>
; Schedule writers, readers, and deferred validators in dependency order.
i1 0 1
i2 1.1 0.01
i7 1.12 0.01
i3 0.2 0.8
i6 1.05 0.01
i4 2 0.01
i5 2.1 0.2
i8 2.31 0.01
e
</CsScore>
</CsoundSynthesizer>
