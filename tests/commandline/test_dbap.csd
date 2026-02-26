<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 8
0dbfs = 1

pos_func@global:i = ftgen:i(0, 0, 16, -2, 0, 0, 45, 0, 90, 0, 135, 0, 180, 0, 225, 0, 270, 0, 315, 0)
w_func@global:i = ftgen:i(0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1)

instr 1
    print(pos_func)
    asig = poscil(1, 440)
    source:k[] = [phasor:k(0.3) * 360, 0.0]
    ; source:k[] = [0.0, 0.0]
    lpos:i[][] = init(8, 2)
    lpos = [0, 0, 45, 0, 90, 0, 135, 0, 180, 0, 225, 0, 270, 0, 315, 0]

    dbap_sig:a[] = init(8)
    dbap_sig = dbap(asig, 1, source, lpos, 3, 24.0) // using arr pos

    dbap_gains:k[] = init(8)
    dbap_gains = dbapgains:k[](1, source, pos_func, 3, 24.0, 2, w_func) // using func pos
    trig:k = metro(10)
    printarray(dbap_gains, trig, "", "gains = ")

    out(dbap_sig[0], dbap_sig[1], dbap_sig[2], dbap_sig[3], dbap_sig[4], dbap_sig[5], dbap_sig[6], dbap_sig[7])


endin

</CsInstruments>
<CsScore>

i 1 0 10

</CsScore>
</CsoundSynthesizer>
