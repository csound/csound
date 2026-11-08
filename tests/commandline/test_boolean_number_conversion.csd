<CsTest>
description = "a boolean read as a number is 0 or 1, and a number read as a boolean is true unless it is 0"

[expect]
exit = 0
output = ["pass init", "pass init a", "pass assign", "pass fillarray", "pass var-arg", "pass schedule", "pass event", "pass B at init", "pass eq i", "pass eq k", "pass switch", "pass xout i", "pass xout k", "pass xout boolean", "pass xout branches", "pass parameter", "pass logic", "pass goto", "pass goto i", "pass k of k", "pass k of k at init", "pass xout late", "pass xout skipped", "pass global b", "pass xout late i", "pass parameter written", "pass parameter not written", "flag = true", "mixed = true"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 32

opcode Number(value:i):b
  xout value
endop

opcode NumberK(value:k):B
  xout value
endop

opcode Pair():(b, b)
  xout 1, 0
endop

opcode Branches(which:i):b
  if which == 1 then
    xout 1
  else
    xout 0
  endif
endop

; a UDO's k and B outputs reach the caller at the end of the k-period, so
; they carry what the variable holds then, wherever the xout is
opcode LateB(value:k):B
  held:k init 0
  xout held
  held = value
endop

opcode LateK(value:k):k
  held:k init 0
  xout held
  held = value
endop

opcode SkippedB(value:k, skip:k):B
  held:k = value
  if skip > 0 kgoto done
  xout held
done:
endop

; and the i and b outputs carry what the variable holds at the end of init
opcode LateNumber(value:i):i
  held:i init 0
  xout held
  held = value
endop

opcode LateBToI(value:i):i
  held:b init 2 < 1
  xout held
  held = value > 0
endop

opcode LateIToB(value:i):b
  held:i init 0
  xout held
  held = value
endop

opcode LocalLateIToB(value:i):b
  setksmps 1
  held:i init 0
  xout held
  held = value
endop

opcode LocalLateNumber(value:i):i
  setksmps 1
  held:i init 0
  xout held
  held = value
endop

; a parameter is passed by reference
opcode SetTrue(state:B):void
  state = 1 < 2
endop

; and what the UDO writes some other way is left as it is
opcode Negated(flag:B):k
  result:k init 0
  if flag then
    result = 0
  else
    result = 1
  endif
  xout result
endop

opcode ClearOther(flag:B, other:k):void
  other = 0
endop

opcode Flipped(flag:B):k
  flag = timeinstk() < 0
  xout 7
endop

opcode ClearSecond(first:B, second:B):void
  second = timeinstk() < 0
endop

opcode Truth():i
  xout true
endop

opcode TruthK():k
  flag:B = 1 < 2
  xout flag
endop

opcode Steered(value:B):k
  steered:k init 0
  if value then
    steered = 1
  else
    steered = 0
  endif
  xout steered
endop

opcode SteeredLocal(value:B):k
  setksmps 1
  steered:k init 0
  if value then
    steered = 1
  else
    steered = 0
  endif
  xout steered
endop

; each output of an opcode that reads numbers holds 1, not the bits of a boolean
instr 1
  flag:b = 1 < 2
  fromkey:i init true
  fromcompare:i init 1 < 2
  fromvar:i init flag
  fromfalse:i init false
  first:i, second:i init true, 0.5
  controlled:k init true
  twice:k, more:k init flag, 0
  if fromkey != 1 || fromcompare != 1 || fromvar != 1 || fromfalse != 0 ||
     first != 1 || second != 0.5 || i(controlled) != 1 || i(twice) != 1 ||
     i(more) != 0 then
    prints("init failed: %g %g %g %g %g %g %g %g %g\n", fromkey, fromcompare,
           fromvar, fromfalse, first, second, i(controlled), i(twice), i(more))
  else
    prints("pass init\n")
  endif
endin

instr 2
  audio:a init true
  if k(audio) == 1 then
    printf("pass init a\n", 1)
  else
    printf("init a failed: %g\n", 1, k(audio))
  endif
  turnoff
endin

instr 3
  flag:B = 1 < 2
  ka:k, kb:k = true, 1
  kc:k, kd:k = flag, 2 < 1
  ia:i, ib:i = true, 3
  if ka == 1 && kb == 1 && kc == 1 && kd == 0 && i(ia) == 1 && i(ib) == 3 then
    printf("pass assign\n", 1)
  else
    printf("assign failed: %g %g %g %g %g %g\n", 1, ka, kb, kc, kd, ia, ib)
  endif
  turnoff
endin

instr 4
  flag:b = 1 < 2
  numbers:i[] fillarray true, 3, flag, 2 < 1
  controls:k[] fillarray true, 3
  sized:i[] init true
  if numbers[0] != 1 || numbers[1] != 3 || numbers[2] != 1 || numbers[3] != 0 ||
     i(controls, 0) != 1 || i(controls, 1) != 3 || lenarray(sized) != 1 then
    prints("fillarray failed: %g %g %g %g %g %g %g\n", numbers[0], numbers[1],
           numbers[2], numbers[3], i(controls, 0), i(controls, 1), lenarray(sized))
  else
    prints("pass fillarray\n")
  endif
endin

instr 5
  flag:b = 1 < 2
  numbers:i[] fillarray 10, 20
  picked:i = numbers[flag]
  biggest:i = max(0.5, flag)
  text:S = sprintf("%g%g", 1, flag)
  if picked != 20 || biggest != 1 || strcmp(text, "11") != 0 then
    prints("var-arg failed: %g %g %s\n", picked, biggest, text)
  else
    prints("pass var-arg\n")
  endif
endin

instr 6
  flag:b = 1 < 2
  schedule(7, 0, 0, flag)
  eventi("i", 8, 0, 0, true)
endin

instr 7
  if p4 == 1 then
    prints("pass schedule\n")
  else
    prints("schedule failed: %g\n", p4)
  endif
endin

instr 8
  if p4 == 1 then
    prints("pass event\n")
  else
    prints("event failed: %g\n", p4)
  endif
endin

; an opcode that runs at init sees what a B holds there, as it would a k
instr 9
  flag:B init 1
  text:S = sprintf("%g%g", 1, flag)
  if strcmp(text, "11") == 0 then
    prints("pass B at init\n")
  else
    prints("B at init failed: %s\n", text)
  endif
endin

instr 10
  yes:b = 1 < 2
  no:b = 2 < 1
  different:b = yes == no
  same:b = yes == yes
  one:b = yes == 1
  flipped:b = 1 == yes
  zero:b = no == 0
  half:b = yes == 0.5
  keywords:b = true == false
  if i(different) != 0 || i(same) != 1 || i(one) != 1 || i(flipped) != 1 ||
     i(zero) != 1 || i(half) != 0 || i(keywords) != 0 then
    prints("eq i failed: %g %g %g %g %g %g %g\n", i(different), i(same), i(one),
           i(flipped), i(zero), i(half), i(keywords))
  else
    prints("pass eq i\n")
  endif
  switch yes
    case 1
      prints("pass switch\n")
    default
      prints("switch failed\n")
  endsw
endin

instr 11
  yes:B = 1 < 2
  no:B = 2 < 1
  one:k = 1
  half:k = 0.5
  zero:k = 0
  different:B = yes == no
  same:B = yes == yes
  number:B = yes == one
  flipped:B = one == yes
  fraction:B = yes == half
  unset:B = no == zero
  atinit:B = yes == true
  keywords:B = truek == falsek
  if k(different) == 0 && k(same) == 1 && k(number) == 1 && k(flipped) == 1 &&
     k(fraction) == 0 && k(unset) == 1 && k(atinit) == 1 && k(keywords) == 0 then
    printf("pass eq k\n", 1)
  else
    printf("eq k failed: %g %g %g %g %g %g %g %g\n", 1, k(different), k(same),
           k(number), k(flipped), k(fraction), k(unset), k(atinit), k(keywords))
  endif
  turnoff
endin

instr 12
  value:b = Number(0.1)
  whole:b = Number(1)
  zero:b = Number(0)
  first:b, second:b = Pair()
  before:i = i(value)
  ; the variable holds no more than the boolean: nothing of the number is left
  value = 2 < 1
  after:b init value
  if before != 1 || i(whole) != 1 || i(zero) != 0 || i(first) != 1 ||
     i(second) != 0 || i(after) != 0 then
    prints("xout i failed: %g %g %g %g %g %g\n", before, i(whole), i(zero),
           i(first), i(second), i(after))
  else
    prints("pass xout i\n")
  endif
endin

instr 13
  one:k = 1
  half:k = 0.5
  zero:k = 0
  first:B = NumberK(one)
  second:B = NumberK(half)
  third:B = NumberK(zero)
  if k(first) == 1 && k(second) == 1 && k(third) == 0 then
    printf("pass xout k\n", 1)
  else
    printf("xout k failed: %g %g %g\n", 1, k(first), k(second), k(third))
  endif
  turnoff
endin

instr 14
  fromtrue:i = Truth()
  fromflag:k = TruthK()
  if fromflag == 1 && i(fromtrue) == 1 then
    printf("pass xout boolean\n", 1)
  else
    printf("xout boolean failed: %g %g\n", 1, fromtrue, fromflag)
  endif
  turnoff
endin

instr 15
  yes:b = Branches(1)
  no:b = Branches(0)
  if i(yes) != 1 || i(no) != 0 then
    prints("xout branches failed: %g %g\n", i(yes), i(no))
  else
    prints("pass xout branches\n")
  endif
endin

instr 16
  one:k = 1
  half:k = 0.5
  zero:k = 0
  if Steered(one) == 1 && Steered(half) == 1 && Steered(zero) == 0 &&
     SteeredLocal(one) == 1 && SteeredLocal(half) == 1 &&
     SteeredLocal(zero) == 0 then
    printf("pass parameter\n", 1)
  else
    printf("parameter failed: %g %g %g %g %g %g\n", 1, Steered(one),
           Steered(half), Steered(zero), SteeredLocal(one), SteeredLocal(half),
           SteeredLocal(zero))
  endif
  turnoff
endin

instr 17
  one:k = 1
  zero:k = 0
  both:B = one && one
  either:B = one || zero
  negated:B = !one
  mixed:B = (2 > 1) && one
  neither:B = zero || zero
  if k(both) == 1 && k(either) == 1 && k(negated) == 0 && k(mixed) == 1 &&
     k(neither) == 0 then
    printf("pass logic\n", 1)
  else
    printf("logic failed: %g %g %g %g %g\n", 1, k(both), k(either), k(negated),
           k(mixed), k(neither))
  endif
  turnoff
endin

instr 18
  one:k = 1
  ckgoto one, jumped
  printf("goto failed\n", 1)
  kgoto done
jumped:
  printf("pass goto\n", 1)
done:
  turnoff
endin

; an init-time goto reads a k as it stands at init
instr 21
  one:k init 1
  cigoto one, jumped
  prints("goto i failed\n")
  igoto done
jumped:
  prints("pass goto i\n")
done:
endin

; k of a number is that number, not what a boolean's slot makes of it
instr 19
  half:k = 0.5
  three:k = 3
  tenth:k = 0.1
  if k(half) == 0.5 && k(three) == 3 && k(tenth) == 0.1 then
    printf("pass k of k\n", 1)
  else
    printf("k of k failed: %g %g %g\n", 1, k(half), k(three), k(tenth))
  endif
  turnoff
endin

instr 22
  one:k init 1
  cigoto k(one), jumped
  prints("k of k at init failed\n")
  igoto done
jumped:
  prints("pass k of k at init\n")
done:
endin

instr 23
  cycle:k init 0
  cycle += 1
  value:k = cycle > 1 ? 1 : 0
  late:B = LateB(value)
  reference:k = LateK(value)
  skipped:B = SkippedB(value, value)
  mismatches:k init 0
  if k(late) != reference then
    mismatches += 1
  endif
  if cycle == 3 then
    if mismatches == 0 then
      printf("pass xout late\n", 1)
    else
      printf("xout late failed: %g\n", 1, mismatches)
    endif
    if k(skipped) == 1 then
      printf("pass xout skipped\n", 1)
    else
      printf("xout skipped failed\n", 1)
    endif
    turnoff
  endif
endin

instr 26
  reference:i = LateNumber(1)
  fromb:i = LateBToI(1)
  tob:b = LateIToB(1)
  localref:i = LocalLateNumber(1)
  localb:b = LocalLateIToB(1)
  if fromb != reference || i(tob) != reference || i(localb) != (localref != 0 ? 1 : 0) then
    prints("xout late i failed: %g %g %g %g %g\n", reference, fromb, i(tob), localref, i(localb))
  else
    prints("pass xout late i\n")
  endif
endin

instr 27
  flag:k init 0
  SetTrue(flag)
  half:k = 0.5
  steered:k = Steered(half)
  if flag == 1 && half == 0.5 && steered == 1 then
    printf("pass parameter written\n", 1)
  else
    printf("parameter written failed: %g %g %g\n", 1, flag, half, steered)
  endif
  turnoff
endin

instr 28
  value:k = 0
  value = Negated(value)
  shared:k = 1
  ClearOther(shared, shared)
  both:k = 1
  ClearSecond(both, both)
  returned:k = 1
  returned = Flipped(returned)
  if value == 1 && shared == 0 && both == 0 && returned == 7 then
    printf("pass parameter not written\n", 1)
  else
    printf("parameter not written failed: %g %g %g %g\n", 1, value, shared, both, returned)
  endif
  turnoff
endin

; an opcode reading a global b at performance sees it change
gstate@global:b = 1 < 2

instr 24
  seen:k = max(0, gstate)
  early:k init -1
  if timeinstk() == 1 then
    early = seen
  endif
  if timeinstk() == 30 then
    if early == 1 && seen == 0 then
      printf("pass global b\n", 1)
    else
      printf("global b failed: %g %g\n", 1, early, seen)
    endif
    turnoff
  endif
endin

instr 25
  gstate = 2 < 1
endin

; print reads a boolean by its type, and names the argument
instr 20
  flag:b = 1 < 2
  number:i = 3
  mixed:b = 1 < 2
  print flag, number, mixed
endin
</CsInstruments>
<CsScore>
i1 0 0
i2 0 0.01
i3 0 0.01
i4 0 0
i5 0 0
i6 0 0
i9 0 0
i10 0 0
i11 0 0.01
i12 0 0
i13 0 0.01
i14 0 0.01
i15 0 0
i16 0 0.01
i17 0 0.01
i18 0 0.01
i21 0 0
i22 0 0
i23 0 0.01
i24 0 0.05
i25 0.01 0
i26 0 0
i27 0 0.01
i28 0 0.01
i19 0 0.01
i20 0 0
e 0.1
</CsScore>
</CsoundSynthesizer>
