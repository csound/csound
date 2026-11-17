<CsTest>
description = "a UDO that keeps state in the variable it returns shares it with the caller, computed argument or not"

[expect]
exit = 0
output = ["shared the state for 3 periods"]
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

opcode Counter(step:k):k
  count:k init 0
  count += step
  xout(count)
endop

instr 1
  period:k init 0
  period += 1
  one:k init 1

  ; the variable the call returns into is the UDO's own, so what the caller
  ; writes to it the UDO adds to next: 1 becomes 100, 101 becomes 10100
  computed:k = Counter(one + 0)
  computed = computed * 100
  plain:k = Counter(one)
  plain = plain * 100

  expected:k = period == 1 ? 100 : (period == 2 ? 10100 : 1010100)
  if computed != expected || plain != expected then
    printks "period %d: computed %g plain %g expected %g\n", 0, period, computed, plain, expected
    exitnowk(-1)
  endif
  if period == 3 then
    printks "shared the state for %d periods\n", 0, period
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
