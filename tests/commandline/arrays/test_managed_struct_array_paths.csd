<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1

struct Label text:S
struct Rack labels:Label[]

opcode Forward(input:Label[]):Label[]
  xout input
endop

instr 1
  source:Label[] init 2
  first:Label init "one"
  second:Label init "two"
  changed:Label init "changed"
  source[0] = first
  source[1] = second

  reshaped:Label[] = Forward(source)
  reshapearray reshaped, 1, 2
  reshaped[0][0] = changed

  rack:Rack init source
  rack.labels[1] = changed

  if lenarray(reshaped, 0) != 2 || \
      lenarray(reshaped, 1) != 1 || \
      lenarray(reshaped, 2) != 2 then
    prints "managed array shape disagrees\n"
    exitnow(1)
  endif
  if strcmp(source[0].text, "one") != 0 || \
      strcmp(source[1].text, "two") != 0 || \
      strcmp(reshaped[0][0].text, "changed") != 0 || \
      strcmp(reshaped[0][1].text, "two") != 0 || \
      strcmp(rack.labels[0].text, "one") != 0 || \
      strcmp(rack.labels[1].text, "changed") != 0 then
    prints "managed array paths disagree\n"
    exitnow(1)
  endif
  prints "managed array paths agree\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
