<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o chnget.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;Example by Joachim Heintz
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1; send k-values
    SChan[] init 2
    SChan[0] = "kChan1"
    SChan[1] = "kChan2"
    kArr[] fillarray 1, 2
    chnsetk kArr, SChan
endin

instr 2; get k-values
    SChan[] init 2
    SChan[0] = "kChan1"
    SChan[1] = "kChan2"
    kIndex = 0

    kVals[] chngetk SChan

    if metro(1) == 1then
        printarray kVals, 1
    endif
endin

instr 3 //send i-values
    SChan[] init 2
    SChan[0] = "iChan1"
    SChan[1] = "iChan2"

    iVals[] fillarray 1, 2
    chnseti  iVals, SChan  
endin


instr 4  // get i-values
    SChan[] init 2
    SChan[0] = "iChan1"
    SChan[1] = "iChan2"

    iVals[] chngeti SChan

    printarray iVals
endin

instr 5 ; send a-values
    SChan[] init 2
    SChan[0] = "aChan1"
    SChan[1] = "aChan2"
    aVals[] init 2
    aVals[0] = oscili:a(1, 1500, -1);
    aVals[1] = oscili:a(1, 300, -1, 0.1);

    chnseta    aVals, SChan

endin


instr 6 ; get a-values  
    setksmps 1
    SChan[] init 2
    SChan[0] = "aChan1"
    SChan[1] = "aChan2"

    aVals[] chngeta SChan

    ;printks "Channel1Val:%f",.01, k(aVals[0])
    ;printks "Channel1Val:%f",.01, k(aVals[1])

    outs aVals[0], aVals[1]*0
    
endin

instr 7 ; send S-values
    SChan[] init 2
    SChan[0] = "sChan1"
    SChan[1] = "sChan2"

    SVal[] init 2
    SVal[0] = "Hello"
    SVal[1] = "There"
    chnsets    SVal, SChan
endin


instr 8 ; get S-values  
    SChan[] init 2
    SChan[0] = "sChan1"
    SChan[1] = "sChan2"

    SVals[] chngets SChan

    prints SVals[0]
    prints SVals[1]
    
endin



</CsInstruments>
<CsScore>
i1 0 .1
i2 1 .1
i3 2 .1
i4 3 .1
i5 4 4
i6 5 4
i7 6 .1
i8 7 .1
</CsScore>
</CsoundSynthesizer>