<CsTest>
description = "a new-style UDO at a local ksmps or sr gives an i or b output again when its body changes it, as without one"

[expect]
exit = 0
output = ["pass reinit i", "pass reinit b", "pass reinit number for b", "pass k for i", "pass k for b", "pass kept", "pass global", "pass member", "pass old-style", "pass two outputs", "pass rebound"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

gkvalue init 0

struct Pair left:k, right:k

opcode AsNumber(flag:b):k
  xout flag
endop

opcode Miss(got:k, want:k):k
  xout got == want ? 0 : 1
endop

opcode Report(name:S, bad:k, local:k):void
  if bad == 0 then
    printf("pass %s\n", 1, name)
  else
    printf("%s failed: %d without a local rate, %d with one\n", 1, name, bad - local, local)
  endif
endop

; A reinit on the third k-period gives the output 3, or true; it is 0, or
; false, from init.
opcode ReinitI(when:k):i
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:i = i(when)
  rireturn
  xout value
endop

opcode ReinitILocal(when:k):i
  setksmps 1
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:i = i(when)
  rireturn
  xout value
endop

; the caller's ksmps, set: this runs by copy too
opcode ReinitISame(when:k):i
  setksmps 32
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:i = i(when)
  rireturn
  xout value
endop

; one variable returned through two outputs
opcode ReinitTwoLocal(when:k):(i, i)
  setksmps 1
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:i = i(when)
  rireturn
  xout value, value
endop

opcode ReinitB(when:k):b
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:b = i(when) > 0
  rireturn
  xout value
endop

opcode ReinitBLocal(when:k):b
  setksmps 1
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:b = i(when) > 0
  rireturn
  xout value
endop

opcode ReinitBSame(when:k):b
  setksmps 32
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:b = i(when) > 0
  rireturn
  xout value
endop

; a number for a b output, converted at the end of the body
opcode ReinitNumber(when:k):b
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:i = i(when)
  rireturn
  xout value
endop

opcode ReinitNumberLocal(when:k):b
  setksmps 1
  done:k init 0
  if when == 3 && done == 0 then
    done = 1
    reinit CHANGE
  endif
CHANGE:
  value:i = i(when)
  rireturn
  xout value
endop

; a k returned through an i or b output
opcode KToI(value:k):i
  xout value
endop

opcode KToILocal(value:k):i
  setksmps 1
  xout value
endop

opcode KToISame(value:k):i
  setksmps 32
  xout value
endop

opcode KToIUnder(value:k):i
  undersample 2
  xout value
endop

; converter 5 holds a k input, so the body reads it at once
opcode KToIOver(value:k):i
  oversample 2, 5
  xout value
endop

opcode KToB(value:k):b
  xout value
endop

opcode KToBLocal(value:k):b
  setksmps 1
  xout value
endop

opcode KToBSame(value:k):b
  setksmps 32
  xout value
endop

; a change the caller makes to the output after the call stays while the
; body does not change its variable: an i, a k set at init, a UDO call's i
; output, an argument returned into the variable the caller passed for it,
; and a variable set again after xout at init
opcode Five(when:k):i
  five:i = 5
  xout five
endop

opcode FiveLocal(when:k):i
  setksmps 1
  five:i = 5
  xout five
endop

opcode Late(when:k):i
  late:i = 1
  xout late
  late = 2
endop

opcode LateLocal(when:k):i
  setksmps 1
  late:i = 1
  xout late
  late = 2
endop

opcode PassI(value:i, when:k):i
  xout value
endop

opcode PassILocal(value:i, when:k):i
  setksmps 1
  xout value
endop

opcode InitK(when:k):i
  held:k init 5
  xout held
endop

opcode InitKLocal(when:k):i
  setksmps 1
  held:k init 5
  xout held
endop

; a UDO call in the body runs during performance, but gives its i output
; at init
opcode Inner(when:k):i
  one:i = 1
  xout one
endop

opcode Nest(when:k):i
  inner:i = Inner(when)
  xout inner
endop

opcode NestLocal(when:k):i
  setksmps 1
  inner:i = Inner(when)
  xout inner
endop

; a global is not the body's own: its value at init
opcode Global(when:k):i
  xout gkvalue
endop

opcode GlobalLocal(when:k):i
  setksmps 1
  xout gkvalue
endop

; a member keeps its struct's storage, and is given at init, as a global
opcode Member(value:k):i
  pair:Pair init 0, 0
  pair.right = value
  xout pair.right
endop

opcode MemberLocal(value:k):i
  setksmps 1
  pair:Pair init 0, 0
  pair.right = value
  xout pair.right
endop

; an old-style UDO runs by copy with or without a local rate: its xout
; gives an i output at init, and a reinit that stops before it leaves it
opcode ReinitOld, i, k
  kwhen xin
  kdone init 0
  if kwhen == 3 && kdone == 0 then
    kdone = 1
    reinit CHANGE
  endif
CHANGE:
  ivalue = i(kwhen)
  rireturn
  xout ivalue
endop

opcode ReinitOldLocal, i, k
  kwhen xin
  setksmps 1
  kdone init 0
  if kwhen == 3 && kdone == 0 then
    kdone = 1
    reinit CHANGE
  endif
CHANGE:
  ivalue = i(kwhen)
  rireturn
  xout ivalue
endop

instr 1
  cycle:k init 0
  cycle += 1
  gkvalue = cycle
  flip:k = cycle % 2
  reinit:k = cycle < 3 ? 0 : 3
  set:k = cycle < 3 ? 0 : 1

  ri:i = ReinitI(cycle)
  riLocal:i = ReinitILocal(cycle)
  riSame:i = ReinitISame(cycle)
  rb:b = ReinitB(cycle)
  rbLocal:b = ReinitBLocal(cycle)
  rbSame:b = ReinitBSame(cycle)
  rn:b = ReinitNumber(cycle)
  rnLocal:b = ReinitNumberLocal(cycle)
  ki:i = KToI(flip)
  kiLocal:i = KToILocal(flip)
  kiSame:i = KToISame(flip)
  kiUnder:i = KToIUnder(flip)
  kiOver:i = KToIOver(flip)
  kb:b = KToB(flip)
  kbLocal:b = KToBLocal(flip)
  kbSame:b = KToBSame(flip)
  late:i = Late(cycle)
  late += 10
  lateLocal:i = LateLocal(cycle)
  lateLocal += 10
  initk:i = InitK(cycle)
  initk += 10
  initkLocal:i = InitKLocal(cycle)
  initkLocal += 10
  nest:i = Nest(cycle)
  nest += 10
  nestLocal:i = NestLocal(cycle)
  nestLocal += 10
  same:i init 3
  same = PassI(same, cycle)
  same += 10
  sameLocal:i init 3
  sameLocal = PassILocal(sameLocal, cycle)
  sameLocal += 10
  five:i = Five(cycle)
  five += 1
  fiveLocal:i = FiveLocal(cycle)
  fiveLocal += 1
  global:i = Global(cycle)
  globalLocal:i = GlobalLocal(cycle)
  first:i, second:i = ReinitTwoLocal(cycle)
  member:i = Member(flip)
  memberLocal:i = MemberLocal(flip)
  old:i ReinitOld cycle
  oldLocal:i ReinitOldLocal cycle

  badri:k init 0
  badriLocal:k init 0
  badrb:k init 0
  badrbLocal:k init 0
  badrn:k init 0
  badrnLocal:k init 0
  badki:k init 0
  badkiLocal:k init 0
  badkb:k init 0
  badkbLocal:k init 0
  badfive:k init 0
  badfiveLocal:k init 0
  badglobal:k init 0
  badtwo:k init 0
  badmember:k init 0
  badold:k init 0
  badoldLocal:k init 0
  badri += Miss(ri, reinit)
  badriLocal += Miss(riLocal, reinit) + Miss(riSame, reinit)
  badrb += Miss(AsNumber(rb), set)
  badrbLocal += Miss(AsNumber(rbLocal), set) + Miss(AsNumber(rbSame), set)
  badrn += Miss(AsNumber(rn), set)
  badrnLocal += Miss(AsNumber(rnLocal), set)
  badki += Miss(ki, flip)
  badkiLocal += Miss(kiLocal, flip) + Miss(kiSame, flip) +
    Miss(kiUnder, flip) + Miss(kiOver, flip)
  badkb += Miss(AsNumber(kb), flip)
  badkbLocal += Miss(AsNumber(kbLocal), flip) + Miss(AsNumber(kbSame), flip)
  badfive += Miss(five, 6) + Miss(initk, 15) + Miss(nest, 11) +
    Miss(same, 13) + Miss(late, 12)
  badfiveLocal += Miss(fiveLocal, 6) + Miss(initkLocal, 15) +
    Miss(nestLocal, 11) + Miss(sameLocal, 13) + Miss(lateLocal, 12)
  badglobal += Miss(globalLocal, global) + Miss(global, 0)
  badtwo += Miss(first, reinit) + Miss(second, reinit)
  badmember += Miss(memberLocal, member)
  badold += Miss(old, 0)
  badoldLocal += Miss(oldLocal, 0)

  if cycle == 5 then
    Report("reinit i", badri + badriLocal, badriLocal)
    Report("reinit b", badrb + badrbLocal, badrbLocal)
    Report("reinit number for b", badrn + badrnLocal, badrnLocal)
    Report("k for i", badki + badkiLocal, badkiLocal)
    Report("k for b", badkb + badkbLocal, badkbLocal)
    Report("kept", badfive + badfiveLocal, badfiveLocal)
    Report("global", badglobal, badglobal)
    Report("member", badmember, badmember)
    Report("old-style", badold + badoldLocal, badoldLocal)
    Report("two outputs", badtwo, badtwo)
    turnoff
  endif
endin
; An Opcode object performed into another variable binds xout's arguments
; again, from the first xout: a global, which stays at init, or a local that
; does not change
opcode TwoXout(when:k):i
  local:i init 7
  xout gkvalue
  xout local
endop

opcode TwoXoutLocal(when:k):i
  setksmps 1
  local:i init 7
  xout gkvalue
  xout local
endop

opcode TwoLocals(when:k):i
  first:i init 1
  second:i init 2
  xout first
  xout second
endop

opcode TwoLocalsLocal(when:k):i
  setksmps 1
  first:i init 1
  second:i init 2
  xout first
  xout second
endop

instr 2
  plain:Opcode[] = create(opcoderef("TwoXout"), 1)
  local:Opcode[] = create(opcoderef("TwoXoutLocal"), 1)
  plainLocals:Opcode[] = create(opcoderef("TwoLocals"), 1)
  localLocals:Opcode[] = create(opcoderef("TwoLocalsLocal"), 1)
  cycle:k init 0
  cycle += 1
  gkvalue = cycle
  plainInit:i init plain[0], cycle
  localInit:i init local[0], cycle
  plainLocalsInit:i init plainLocals[0], cycle
  localLocalsInit:i init localLocals[0], cycle
  plainOther:i init 5
  localOther:i init 5
  plainOther perf plain[0], cycle
  localOther perf local[0], cycle
  plainLocalsOther:i init 5
  localLocalsOther:i init 5
  plainLocalsOther perf plainLocals[0], cycle
  localLocalsOther perf localLocals[0], cycle
  bad:k init 0
  badLocal:k init 0
  bad += Miss(plainOther, 5) + Miss(plainLocalsOther, 5)
  badLocal += Miss(localOther, 5) + Miss(localLocalsOther, 5)
  if cycle == 5 then
    Report("rebound", bad + badLocal, badLocal)
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i1 0 0.01
i2 0 0.01
e 0.1
</CsScore>
</CsoundSynthesizer>
