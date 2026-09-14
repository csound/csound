<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "allow global k-rate struct member with local ksmps",
  "expect": {
    "exit": 0
  }
}
*/
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

struct GlobalBus signal:a, level:k
bus@global:GlobalBus = init()

opcode WriteGlobalControlMember, 0, 0
  setksmps 1
  bus.level = 1
endop

instr 1
  WriteGlobalControlMember
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
