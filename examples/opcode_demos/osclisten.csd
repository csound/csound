<CsoundSynthesizer>
-m3 -otest2
<CsInstruments>

giport1 OSCinit 7770
giport2 OSCinit 7771

    	instr 1
kf1     init 0.0
Sf2     =  ""
ktrig   init 1
kk      OSClisten   giport1, "/foo/bar", "fs", kf1, Sf2
        printf 	    "Port 7770, /foo/bar:  %f, %s\n", kk * ktrig, kf1, Sf2
kk      OSClisten   giport1, "/foo/blah", "fs", kf1, Sf2
        printf 	    "Port 7770, /foo/blah: %f, %s\n", kk * ktrig, kf1, Sf2
kk      OSClisten   giport2, "/foo/bar", "fs", kf1, Sf2
        printf 	    "Port 7771, /foo/bar:  %f, %s\n", kk * ktrig, kf1, Sf2
ktrig   =  	    3 - ktrig    ; make sure that it always changes
    	endin

</CsInstruments>

<CsScore>
i 1 0 10
e
</CsScore>

</CsoundSynthesizer>
