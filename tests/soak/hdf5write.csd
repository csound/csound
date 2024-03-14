<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
nchnls = 2
0dbfs = 1
ksmps = 8
sr = 44100

instr hdf5write

    aArray[] init 2,2 ; Initialise a 2 X 2 a-rate array

    aArray[0][0] vco2 0.2, 100 ; Fill array with vco2 signals
    aArray[0][1] vco2 0.4, 200
    aArray[1][0] vco2 0.8, 300
    aArray[1][1] vco2 1, 400

    aVar vco2 0.2, 100 ; Initialise an a-rate variable with a vco2 signal

    kVar phasor 1 ; Initalise a k-rate variable with a phasor signal

    hdf5write "example.h5", aArray, aVar, kVar ; Write variables to an hdf5 file

endin

</CsInstruments>
<CsScore>

i "hdf5write" 0 1

</CsScore>
</CsoundSynthesizer>
