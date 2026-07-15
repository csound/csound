<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

struct Sample value:i

opcode ForwardSamples(input:Sample[]):Sample[]
  xout input
endop

instr 1
  source:Sample[] init 4
  first:Sample init 10
  second:Sample init 20
  third:Sample init 30
  fourth:Sample init 40

  source[0] = first
  source[1] = second
  source[2] = third
  source[3] = fourth

  ; A UDO return shares structured backing storage until the first write.
  ; This specifically exercises reshapearray's detach path.
  reshaped:Sample[] = ForwardSamples(source)
  reshapearray reshaped, 2, 2

  if lenarray(source, 0) != 1 || lenarray(source, 1) != 4 then
    prints "reshaping a copy changed the source array\n"
    exitnow(1)
  endif
  if source[3].value != 40 then
    prints "reshaping a copy changed a source element\n"
    exitnow(1)
  endif

  if lenarray(reshaped, 0) != 2 || lenarray(reshaped, 1) != 2 || \
      lenarray(reshaped, 2) != 2 then
    prints "reshaped struct array has invalid metadata\n"
    exitnow(1)
  endif
  if reshaped[1][1].value != 40 then
    prints "reshaped struct array has invalid values\n"
    exitnow(1)
  endif

  replacement:Sample init 99
  reshaped[0][0] = replacement
  if reshaped[0][0].value != 99 || source[0].value != 10 then
    prints "reshaped struct array still aliases its source\n"
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
