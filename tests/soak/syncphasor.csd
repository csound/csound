<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o abs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

instr 1
    ; Use two syncphasors - one is the "master",
    ; the other the "slave"
    
    ; master's frequency determines pitch
    imastercps  =           cpspch(p4)
    imaxamp     =           10000

    ; the slave's frequency affects the timbre 
    kslavecps   line        imastercps, p3, imastercps * 3
    
    ; the master "oscillator"
    ; the master has no sync input 
    anosync     init        0.0
    am, async   syncphasor  imastercps, anosync
    
    ; the slave "oscillator"
    aout, as    syncphasor  kslavecps, async
    
    adeclick    linseg      0.0, 0.05, 1.0, p3 - 0.1, 1.0, 0.05, 0.0
    
    ; Output the slave's phase value which is a rising
    ; sawtooth wave.  This produces aliasing, but hey, this
    ; this is just an example ;)
    
              out         aout * adeclick * imaxamp
endin

</CsInstruments>
<CsScore>

i1 0 1    7.00
i1 + 0.5  7.02
i1 + .    7.05
i1 + .    7.07
i1 + .    7.09
i1 + 2    7.06

e

</CsScore>
</CsoundSynthesizer>
