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

  instr 3; send values

    kfreq     = 10
    kIndex = 0
    //k-rate channels
    SChan[] init 2
    SChan[0] = "channel1"
    SChan[1] = "channel2"
    while kIndex < 2 do
      chnset    kfreq, SChan[kIndex]
      kIndex+=1
    od

    //S channels
    SChan1[] init 2
    SChan1[0] = "Schannel1"
    SChan1[1] = "Schannel2"
    SChan2[] init 2
    SChan2[0] = "to "
    SChan2[1] = "everyone "
    kIndex = 0
    while kIndex < 2 do
      chnset    SChan2[kIndex], SChan1[kIndex]
      kIndex+=1
    od
  endin

  instr 30; pick up k-values
    SChan[] init 2
    SChan[0] = "channel1"
    SChan[1] = "channel2"
    kIndex = 0
    k1 = 0

    while kIndex<2 do
        k1 += chnget:k(SChan[kIndex])
        kIndex+=1
    od
        printk 0, k1
  endin

  instr 40; pick up a-values

    SStr init "Hello "
    SChan[] init 2
    SChan[0] = "Schannel1"
    SChan[1] = "Schannel2"
    kIndex = 0

    while kIndex<2 do
        SString = chnget:S(SChan[kIndex])
        SStr	    strcatk   SStr, SString
        kIndex+=1
    od

    printks "%s", .2, SStr
  endin


</CsInstruments>
<CsScore>
i3 0 10
i30 1 .5
i40 1 .5
</CsScore>
</CsoundSynthesizer>
