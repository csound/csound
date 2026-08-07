<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>

sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

struct Bus signal:a
bus@global:Bus = init()

opcode ReadBus, a, 0
  setksmps 4
  local:Bus = bus
  xout local.signal
endop

instr 1
  result:a ReadBus
  out result
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
