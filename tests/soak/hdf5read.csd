<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
nchnls = 2
0dbfs = 1
ksmps = 8
sr = 44100

instr hdf5read

    aArray[], aVar, kVar hdf5read "example.h5", "aArray", "aVar", "kVar" ; Open hdf5 file and read variables

    aLeft = (aArray[0][0] + aArray[0][1] + aVar) / 3 ; Add audio signals together for stereo out
    aRight = (aArray[1][0] + aArray[1][1] + aVar) / 3

    outs aLeft * kVar, aRight * kVar ; Multiply audio signals by k-rate signal
endin

</CsInstruments>
<CsScore>

i "hdf5read" 0 1

</CsScore>
</CsoundSynthesizer>
