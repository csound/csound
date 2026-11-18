<CsTest>
description = "a new-style UDO's xout converts a number for an a output, and an a for a k output, as = does"

[expect]
exit = 0
output = ["pass k variable", "pass k input", "pass i", "pass constant", "pass p-field", "pass b", "pass B", "pass a for k", "pass mixed", "pass local ksmps", "pass as it is"]
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

struct Pair left:k, right:k

; the samples of sig that are not value
opcode Mismatches(sig:a, value:k):k
  count:k = 0
  n:k = 0
  while n < ksmps do
    if vaget(n, sig) != value then
      count += 1
    endif
    n += 1
  od
  xout count
endop

opcode Report(name:S, bad:k):void
  if bad == 0 then
    printf("pass %s\n", 1, name)
  else
    printf("%s failed: %d\n", 1, name, bad)
  endif
endop

; a block whose samples are 1, 2, 3, ...
opcode Ramp():a
  block:a init 0
  n:k = 0
  while n < ksmps do
    vaset(n + 1, n, block)
    n += 1
  od
  xout block
endop

opcode FromK(value:k):a
  held:k = value
  xout held
endop

opcode PassK(value:k):a
  xout value
endop

opcode FromI(value:i):a
  xout value
endop

opcode FromConstant():a
  xout 0.25
endop

opcode FromPfield():a
  xout p4
endop

opcode FromB(value:i):a
  flag:b = value > 0
  xout flag
endop

opcode FromBk(value:k):a
  flag:B = value > 0
  xout flag
endop

opcode ToK():k
  block:a = Ramp()
  xout block
endop

opcode Mixed(value:k):(a, k, a)
  block:a = Ramp()
  xout value, block, 3
endop

opcode Local(value:k):a
  setksmps 4
  xout value
endop

; what was taken as it is stays so
opcode IToK(value:i):k
  xout value
endop

opcode KToI(value:k):i
  xout value
endop

opcode KToIArray():i[]
  values:k[] = fillarray(4, 5)
  xout values
endop

opcode IToKArray():k[]
  values:i[] = fillarray(6, 7)
  xout values
endop

opcode KToBArray():b[]
  values:k[] = fillarray(1, 0, 0.5)
  xout values
endop

opcode IToKBArray():B[]
  values:i[] = fillarray(0, 1)
  xout values
endop

; each member's first sample
opcode AToKArray():k[]
  blocks:a[] init 2
  block:a = Ramp()
  blocks[1] = block
  xout blocks
endop

opcode SamePair():Pair
  pair:Pair init 8, 9
  xout pair
endop

opcode BToB(value:i):B
  flag:b = value > 0
  xout flag
endop

instr 1
  cycle:k init 0
  cycle += 1
  value:k = cycle * 0.5
  fromk:a = FromK(value)
  passk:a = PassK(value)
  fromi:a = FromI(0.75)
  fromc:a = FromConstant()
  fromp:a = FromPfield()
  fromb:a = FromB(1)
  frombk:a = FromBk(1)
  first:k = ToK()
  guard:k init 300
  mixeda:a, mixedk:k, mixedc:a = Mixed(value)
  locala:a = Local(value)

  badk:k init 0
  badpass:k init 0
  badi:k init 0
  badc:k init 0
  badp:k init 0
  badb:k init 0
  badbk:k init 0
  badtok:k init 0
  badmixed:k init 0
  badlocal:k init 0
  badk += Mismatches(fromk, value)
  badpass += Mismatches(passk, value)
  badi += Mismatches(fromi, 0.75)
  badc += Mismatches(fromc, 0.25)
  badp += Mismatches(fromp, p4)
  badb += Mismatches(fromb, 1)
  badbk += Mismatches(frombk, 1)
  if first != 1 || guard != 300 then
    badtok += 1
  endif
  badmixed += Mismatches(mixeda, value) + Mismatches(mixedc, 3)
  if mixedk != 1 then
    badmixed += 1
  endif
  badlocal += Mismatches(locala, value)

  if cycle == 3 then
    Report("k variable", badk)
    Report("k input", badpass)
    Report("i", badi)
    Report("constant", badc)
    Report("p-field", badp)
    Report("b", badb)
    Report("B", badbk)
    Report("a for k", badtok)
    Report("mixed", badmixed)
    Report("local ksmps", badlocal)
    turnoff
  endif
endin

instr 2
  ik:k = IToK(2)
  ki:i = KToI(3)
  kiarray:i[] = KToIArray()
  ikarray:k[] = IToKArray()
  pair:Pair = SamePair()
  flag:B = BToB(1)
  flags:b[] = KToBArray()
  kflags:B[] = IToKBArray()
  firsts:k[] = AToKArray()
  first:b = flags[0]
  second:b = flags[1]
  third:b = flags[2]
  kfirst:B = kflags[0]
  ksecond:B = kflags[1]
  numbers:k = ik == 2 && ki == 3 ? 1 : 0
  arrays:k = kiarray[1] == 5 && ikarray[1] == 7 ? 1 : 0
  others:k = pair.left == 8 && pair.right == 9 && k(flag) == 1 ? 1 : 0
  booleans:k = i(first) == 1 && i(second) == 0 && i(third) == 1 && k(kfirst) == 0 && k(ksecond) == 1 ? 1 : 0
  audio:k = firsts[0] == 0 && firsts[1] == 1 ? 1 : 0
  if numbers == 1 && arrays == 1 && others == 1 && booleans == 1 && audio == 1 then
    printf("pass as it is\n", 1)
  else
    printf("as it is failed: %g %g %g %g %g %g %g %g %g\n", 1, ik, ki, kiarray[1], ikarray[1], pair.left, pair.right, k(flag), booleans, audio)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0.01 0.125
i2 0 0.01
e 0.1
</CsScore>
</CsoundSynthesizer>
