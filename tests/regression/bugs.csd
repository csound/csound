<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

instr baz

asig        vibes        .8, 440, .5, p4 , 1, p5, .7, 2, .1
            outs         asig, asig
            print        taninv2(1,2)
            print        tan (1/2)
            ii           taninv2  1,2
            print        ii
            prints       p6
            prints       p7
endin

</CsInstruments>
<CsScore>

; Table #1, the "marmstk1.wav" audio file.
f 1 0 256 1 "marmstk1.wav" 0 0 0
f 2 0 4096 10 1
i "baz" 0 1 0.561 0.1 "foo" "bar"
e
</CsScore>

</CsoundSynthesizer>
