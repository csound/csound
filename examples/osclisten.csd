<CsoundSynthesizer>

<CsInstruments>

giport1 OSCinit 7770
giport2 OSCinit 7771

    instr 2
kf1     init 0
kf2     init 0
Sf3     =  ""
ktrig   init 1
kk      OSClisten   giport1, "/foo/bar", "ifs", kf1, kf2, Sf3
        printf "Port 7770, /foo/bar:  %d, %f, %s\n", kk * ktrig, kf1, kf2, Sf3
kk      OSClisten   giport1, "/foo/blah", "ifs", kf1, kf2, Sf3
        printf "Port 7770, /foo/blah: %d, %f, %s\n", kk * ktrig, kf1, kf2, Sf3
kk      OSClisten   giport2, "/foo/bar", "ifs", kf1, kf2, Sf3
        printf "Port 7771, /foo/bar:  %d, %f, %s\n", kk * ktrig, kf1, kf2, Sf3
ktrig   =  3 - ktrig    ; make sure that it always changes
    endin

</CsInstruments>

<CsScore>
i2 0 30
e
</CsScore>

</CsoundSynthesizer>
