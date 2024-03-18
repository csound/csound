<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in   No messages  MIDI in
-odac           -iadc    ; -d         -M0  ;;;RT audio I/O with MIDI in
; For Non-realtime ouput leave only the line below:
;-o pindex.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;Example by Anthony Kozar Dec 2006

instr 1
    inum    pcount
    index   init 1
    loop1:
        ivalue pindex index
        printf_i "p%d = %f\n", 1, index, ivalue
        index   = index + 1
    if  (index <= inum) igoto loop1
    print inum
endin

</CsInstruments>
<CsScore>
i1  0 3 40 50         ; has 5 pfields
i1  1 2 80            ; has 5 due to carry
i1  2 1 40 50 60 70   ; has 7
e
</CsScore>
</CsoundSynthesizer>