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

sampleDimensions@global:i[] fillarray 1
chnarray "sample-channel", 3, "Sample", sampleDimensions
chnarray "growth-channel", 3, "Sample", sampleDimensions

opcode ForwardSamples(input:Sample[]):Sample[]
  xout input
endop

opcode ReplaceFirstCopy(input:Sample[], replacement:Sample):Sample[]
  setksmps 5
  index:k init 0
  input[index] = replacement
  xout input
endop

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

instr 3
  values:Sample[] init 1
  original:Sample init 10
  values[0] = original
  source:Holder init values
  copied:Holder = source

  replacement:Sample init 77
  index:k init 0
  copied.samples[index] = replacement
  if copied.samples[index].value != 77 || \
      source.samples[index].value != 10 then
    printks "nested struct copy failed: copied=%d source=%d\n", 0, \
      copied.samples[index].value, source.samples[index].value
    exitnowk(1)
  endif
  turnoff
endin

instr 4
  source:Sample[] init 1
  original:Sample init 10
  source[0] = original
  copied:Sample[] = source

  replacement:Sample init 88
  index:k init 0
  source[index] = replacement
  if copied[index].value != 10 || source[index].value != 88 then
    printks "reverse struct copy failed: copied=%d source=%d\n", 0, \
      copied[index].value, source[index].value
    exitnowk(1)
  endif
  turnoff
endin

instr 5
  source:Sample[] init 2
  first:Sample init 10
  second:Sample init 20
  source[0] = first
  source[1] = second
  trimmed:Sample[] = ForwardSamples(source)

  requestedSize:k init 1
  trim trimmed, requestedSize
  trimmedLength:k = lenarray:k(trimmed)
  sourceLength:k = lenarray:k(source)
  if trimmedLength != 1 || sourceLength != 2 || \
      trimmed[0].value != 10 || source[1].value != 20 then
    printks "struct trim failed: trimmed=%d source=%d\n", 0, \
      trimmedLength, sourceLength
    exitnowk(1)
  endif
  turnoff
endin

instr 6
  source:Sample[] init 1
  original:Sample init 10
  source[0] = original
  chnset source, "sample-channel"
  copied:Sample[] chnget "sample-channel"

  replacement:Sample init 66
  index:k init 0
  copied[index] = replacement
  if copied[index].value != 66 || source[index].value != 10 then
    printks "struct channel copy failed: copied=%d source=%d\n", 0, \
      copied[index].value, source[index].value
    exitnowk(1)
  endif
  turnoff
endin

instr 7
  source:Sample[] init 1
  original:Sample init 10
  replacement:Sample init 99
  source[0] = original

  copied:Sample[] = ReplaceFirstCopy(source, replacement)
  index:k init 0
  if source[index].value != 10 || copied[index].value != 99 then
    printks "UDO struct-array copy failed: source=%d copied=%d\n", 0, \
      source[index].value, copied[index].value
    exitnowk(1)
  endif
endin

instr 8
  source:Sample[] init 2
  first:Sample init 10
  second:Sample init 20
  source[0] = first
  source[1] = second

  ; Start the channel and receiver at one element while retaining capacity for
  ; the second source element. The first k-cycle then exercises channel growth.
  trim source, 1
  requestedSize:k init 1
  trim source, requestedSize
  chnset source, "growth-channel"
  received:Sample[] chnget "growth-channel"

  cycle:k init 0
  if cycle == 0 then
    requestedSize = 2
  elseif cycle == 1 then
    if lenarray(received) != 2 || received[0].value != 10 || \
        received[1].value != 20 then
      printks "struct channel growth failed: len=%d first=%d second=%d\n", \
        0, lenarray(received), received[0].value, received[1].value
      exitnowk(1)
    endif
    turnoff
  endif
  cycle += 1
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.1
i 3 0 0.1
i 4 0 0.1
i 5 0 0.1
i 6 0 0.1
i 7 0 0.1
i 8 0 0.1
</CsScore>
</CsoundSynthesizer>
