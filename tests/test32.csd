<CsoundSynthesizer>

<CsInstruments>
instr 1

isrc = 1

if isrc \
     == 1  \
     goto noise
print isrc
goto \
    next
noise: asig rand 1
next:

endin

</CsInstruments>

<CsScore>
e
</CsScore>

</CsoundSynthesizer>
