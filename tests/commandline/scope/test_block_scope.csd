<CsTest>
description = "Typed declarations inside control-flow blocks are scoped to the block"
[expect]
exit = 0
output = ["block scope ok"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode check(got:i, expected:i, what:S):void
  if got != expected then
    prints("FAIL %s: got %d, expected %d\n", what, got, expected)
    exitnow(-1)
  endif
endop

failures@global:k init 0

opcode kcheck(got:k, expected:k, what:S):void
  if got != expected then
    printf("FAIL %s: got %d, expected %d\n", 1, what, got, expected)
    failures += 1
  endif
endop

opcode pick(n:i):i
  r:i init 0
  if n == 1 then
    t:i = 99
    r = t
  endif
  xout(r)
endop

instr 1
  ; declared before the block and assigned inside: visible after it
  acc:i init 0
  i:i init 0
  while i < 3 do
    step:i = i * 2
    acc += step
    i += 1
  od
  check(acc, 6, "while body local")

  ; sibling branches can declare the same name with different types
  res:i init 0
  if p4 == 1 then
    v:i = 10
    res = v
  elseif p4 == 2 then
    v:S = "two"
    res = strlen(v)
  else
    v:i = 30
    res = v
  endif
  check(res, 10, "if branches")

  ; nested blocks see the variables of enclosing blocks
  outer:i init 0
  if p4 == 1 then
    base:i = 5
    j:i = 0
    while j < 2 do
      base += 1
      j += 1
    od
    outer = base
  endif
  check(outer, 7, "nested blocks")

  ; a typed declaration of a visible name keeps referring to it
  shared:i = 1
  if p4 == 1 then
    shared:i = 2
  endif
  check(shared, 2, "outer redeclaration")

  ; implicitly typed variables are not block scoped
  if p4 == 1 then
    ilegacy = 42
  endif
  check(ilegacy, 42, "implicit variable")

  ; for loop body locals
  total:i = 0
  for x in [1, 2, 3] do
    sq:i = x * x
    total += sq
  od
  check(total, 14, "for body")

  ; an existing variable used as loop variable stays visible
  last:i init 0
  for last in [4, 5, 6] do
    total += 1
  od
  check(last, 6, "existing loop variable")

  ; loop and index variables can be reused after the loop
  for y, n in [7, 8] do
    total = y + n
  od
  check(total, 9, "for index variable")
  y:i = 1
  check(y, 1, "loop variable redeclared after loop")

  ; the same name can be declared again after the block
  if p4 == 1 then
    tmp:i = 1
  endif
  tmp:i = 9
  check(tmp, 9, "redeclaration after block")

  ; switch cases
  switch p4
  case 1
    c:i = 11
    acc = c
  default
    c:i = 0
    acc = c
  endsw
  check(acc, 11, "switch case local")

  ; break and continue inside a scoped loop
  count:i = 0
  for z in [1, 2, 3, 4, 5] do
    half:i = z / 2
    if z == 2 then
      continue
    endif
    if half >= 2 then
      break
    endif
    count += z
  od
  check(count, 4, "break and continue")

  ; arrays declared in a block
  if p4 == 1 then
    arr:i[] = [3, 4]
    acc = arr[1]
  endif
  check(acc, 4, "block array")

  ; a name declared with the same type in every branch of a complete
  ; if/elseif/else is visible after it
  if p4 == 1 then
    m:i = 1
  elseif p4 == 2 then
    m:i = 2
  else
    m:i = 3
  endif
  check(m, 1, "declared in every branch")

  ; nested complete ifs merge outwards
  if p4 >= 0 then
    if p4 == 1 then
      deep:i = 10
    else
      deep:i = 20
    endif
  else
    deep:i = 30
  endif
  check(deep, 10, "nested merge")

  ; same for a switch with a default
  switch p4
  case 1
    sw:i = 100
  case 2
    sw:i = 200
  default
    sw:i = 300
  endsw
  check(sw, 100, "switch merge")

  ; UDO body
  check(pick(1), 99, "udo block local")
  check(pick(0), 0, "udo block local not taken")
endin

instr 2
  ; k-rate blocks, including an audio-rate local
  cnt:k init 0
  if timeinstk() <= 2 then
    kl:k = 5
    cnt += kl
  endif
  lvl:k init 0
  if p4 == 1 then
    sig:a = 0.25
    lvl = downsamp(sig)
  endif
  if timeinstk() == 4 then
    kcheck(cnt, 10, "k-rate block")
    kcheck(lvl * 4, 1, "a-rate block local")
  endif
endin

instr 3
  ; the pattern from the issue, written with the declaration outside the
  ; loop: reused instances get the right value
  t:i init 0
  i:i init 0
  while i < p4 do
    t = 42
    i += 1
  od
  check(t, p4 == 1 ? 42 : 0, "instance reuse")
endin

instr 99
  if i(failures) != 0 then
    exitnow(-1)
  endif
  prints("block scope ok\n")
endin

</CsInstruments>
<CsScore>
i1 0 0.1 1
i2 0 0.1 1
i3 0 0.1 1
i3 0.2 0.1 0
i99 0.4 0.01
</CsScore>
</CsoundSynthesizer>
