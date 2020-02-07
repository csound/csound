<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

instr 1

Seval = p4
icompiled compilestr Seval

if (icompiled == 0) then
   printf_i "Compiled: %s\n", 1, Seval
else
   printf_i "!!!Did not compile: %s\n", 1, Seval
endif

endin


schedule 1, 1, 1, {{
instr 10

aL chnget "outLeft"
aR chnget "outRight"

outs aL, aR

chnclear "outLeft"
chnclear "outRight"

endin
}}

schedule 1, 2, 1, {{
schedule 10, 0, -1
}}


schedule 1, 3, 1, {{
instr 20
ipan = p6
ares oscil p4, p5

aleft = ares * ipan
aright = ares * (1 - ipan)

chnmix  aleft, "outLeft"
chnmix  aright, "outRight"

endin
}}


schedule 1, 4, 1, {{
schedule 20, 0, 1, 0.8, 261, 1
}}

schedule 1, 5, 1, {{
schedule 20, 0, 1, 0.8, 330, 0
}}

schedule 1, 6, 1, {{
schedule 20, 0, 1, 0.8, 440, 1
}}


</CsInstruments>
<CsScore>
;i1 0 1

f0 8
e
</CsScore>
</CsoundSynthesizer>
