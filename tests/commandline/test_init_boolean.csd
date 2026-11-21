<CsTest>
description = "init and fillarray of b and B variables read a boolean value as a boolean"

[expect]
exit = 0
output = ["pass global", "pass i", "pass k", "pass udo", "pass fillarray", "pass B fillarray"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 32

opcode Kept(value:b):b
  kept:b init value
  xout kept
endop

; xout of a number from a boolean UDO: 1 is true, though the low half of 1.0 is 0
opcode Number():b
  xout 1
endop

atstart:b init 1 < 2
if atstart then
  prints("pass global\n")
endif

instr 1
  source:b = 1 < 2
  compared:b init 1 < 2
  copied:b = init(source)
  keyword:b init true
  logical:b init !(2 < 1) && source
  steered:i init 0
  if compared then
    steered += 1
  endif
  if copied then
    steered += 1
  endif
  if keyword then
    steered += 1
  endif
  if logical then
    steered += 1
  endif

  ; a false value stays false, and a number is still read as a number
  untrue:b init 2 < 1
  unset:b init false
  half:b init 0.5
  zero:b init 0

  ; each output takes its own value, and the last value fills the rest
  first:b, second:b init 1 < 2, 2 < 1
  third:b, fourth:b init 1 < 2
  fifth:b, sixth:b init 0, 1 < 2

  if steered != 4 || i(untrue) != 0 || i(unset) != 0 || i(half) != 1 || i(zero) != 0 ||
     i(first) != 1 || i(second) != 0 || i(third) != 1 || i(fourth) != 1 ||
     i(fifth) != 0 || i(sixth) != 1 then
    prints("i failed: steered=%g untrue=%g unset=%g half=%g zero=%g outputs=%g%g%g%g%g%g\n",
           steered, i(untrue), i(unset), i(half), i(zero), i(first), i(second),
           i(third), i(fourth), i(fifth), i(sixth))
  else
    prints("pass i\n")
  endif
endin

instr 2
  source:b = 1 < 2
  compared:B init 1 < 2
  copied:B = init(source)
  keyword:B init true
  untrue:B init 2 < 1
  periods:k init 0
  steered:k init 0
  periods += 1
  if compared then
    steered += 1
  endif
  if copied then
    steered += 1
  endif
  if keyword then
    steered += 1
  endif
  if untrue then
    steered += 100
  endif
  if periods == 3 then
    if steered == 9 then
      printf("pass k\n", 1)
    else
      printf("k failed: steered=%g\n", 1, steered)
    endif
    turnoff
  endif
endin

instr 3
  source:b = 1 < 2
  if Kept(source) && Kept(1 < 2) && !Kept(2 < 1) && Kept(Number()) then
    prints("pass udo\n")
  else
    prints("udo failed\n")
  endif
endin

instr 4
  filled:b[] fillarray 1 < 2, 2 < 1, true, 1, 0
  e0:b = filled[0]
  e1:b = filled[1]
  e2:b = filled[2]
  e3:b = filled[3]
  e4:b = filled[4]
  if i(e0) != 1 || i(e1) != 0 || i(e2) != 1 || i(e3) != 1 || i(e4) != 0 then
    prints("fillarray failed: %g%g%g%g%g\n", i(e0), i(e1), i(e2), i(e3), i(e4))
  else
    prints("pass fillarray\n")
  endif
endin

; init refuses a B argument, and fillarray takes one, at init
instr 5
  flag:B = 1 < 2
  filled:B[] fillarray flag, 1 < 2, 2 < 1
  e0:B = filled[0]
  e1:B = filled[1]
  e2:B = filled[2]
  if k(e0) == 1 && k(e1) == 1 && k(e2) == 0 then
    printf("pass B fillarray\n", 1)
  else
    printf("B fillarray failed: %g%g%g\n", 1, k(e0), k(e1), k(e2))
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0
i2 0 0.01
i3 0 0
i4 0 0
i5 0 0.01
</CsScore>
</CsoundSynthesizer>
