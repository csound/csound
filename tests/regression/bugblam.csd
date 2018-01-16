<CsoundSynthesizer>

<CsInstruments>
instr 1
#define BLAM(N) # i$N = 11 #

$BLAM(2)
print i2
endin

instr 2
#define BLAH(N) # i$N. = 12#

$BLAH.(3)
print i3
endin


</CsInstruments>

<CsScore>
i1 0 0
i2 1 0
e

</CsScore>

</CsoundSynthesizer>
