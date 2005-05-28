<CsoundSynthesizer>

<CsOptions>

</CsOptions>

<CsInstruments>
sr=44100
kr=441
ksmps=100
nchnls=2

giFluidEngine           fluid_engine

giSF_piano      fluid_load  "sf2/Piano Grand Steinway V1.0b (29,651KB).SF2", giFluidEngine
giSF_mutedViolin     fluid_load  "sf2/MutedViolin.sf2", giFluidEngine

fluid_program_select    giFluidEngine, 0, giSF_piano, 0, 0
fluid_program_select    giFluidEngine, 1, giSF_mutedViolin, 0, 0

        instr 1 ;untitled

    fluid_play  giFluidEngine, 0, p4, p5

        endin

    instr 2

    fluid_play  giFluidEngine, 1, p4, p5

    endin

    instr 10 ; fluid out instrument

aleft, aright   fluid_out   giFluidEngine

    outs aleft * 0dbfs, aright * 0dbfs
    endin

</CsInstruments>

<CsScore>

i1 0 2 60 110
i1 0 2 64 110
i1 0 2 67 110

i2 0 1 72 110
i2 0 1 76 70
i2 0 1 79 80

i10 0 6

e

</CsScore>

</CsoundSynthesizer>
