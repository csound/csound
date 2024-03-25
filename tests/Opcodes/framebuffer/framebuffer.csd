<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
nchnls = 2
0dbfs = 1
ksmps = 128
sr = 44100

instr 1
    isize init 1024
    ioverlaps init 4

    asig diskin2 "fox.wav", 1, 0, 1
    kframe[] framebuffer asig, isize
    kwindowedFrame[] window kframe, isize

    aout olabuffer kwindowedFrame, ioverlaps
    aout = aout / 2

    outs aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0 400
</CsScore>
</CsoundSynthesizer>
