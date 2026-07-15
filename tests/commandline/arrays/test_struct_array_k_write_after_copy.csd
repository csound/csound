<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

struct Sample value:k
struct Holder samples:Sample[]

instr 1
  source:Sample[] init 2
  initial:Sample init 10
  tracking:Sample init 20
  source[0] = initial
  source[1] = tracking

  replacement:Sample init 99
  index:k init 0
  trackingIndex:k init 1
  cycle:k init 0

  initial.value = 10 + cycle
  source[index] = initial
  tracking.value = 20 + cycle
  source[trackingIndex] = tracking
  copied:Sample[] = source
  replacement.value = 99 + cycle
  copied[index] = replacement
  if copied[index].value != 99 + cycle || \
      source[index].value != 10 + cycle || \
      copied[trackingIndex].value != 20 + cycle then
    printks "k-rate struct-array copy failed: copied=%d source=%d tracked=%d\n", \
      0, copied[index].value, source[index].value, \
      copied[trackingIndex].value
    exitnowk(1)
  endif
  cycle += 1
  if cycle == 3 then
    turnoff
  endif
endin

instr 2
  source:Sample[] init 1
  original:Sample init 10
  source[0] = original
  holder:Holder init source

  replacement:Sample init 77
  index:k init 0
  holder.samples[index] = replacement
  if holder.samples[index].value != 77 || source[index].value != 10 then
    printks "k-rate member-array copy failed: copied=%d source=%d\n", 0, \
      holder.samples[index].value, source[index].value
    exitnowk(1)
  endif
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.1
</CsScore>
</CsoundSynthesizer>
