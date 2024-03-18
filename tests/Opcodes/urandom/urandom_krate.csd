<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac      ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o urandom_krate.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 ; Create random numbers at k-rate in the range -1 to 1 at krate
   
k1 urandom                              ; with a uniform distribution.
printks "k1=%f\\n", 0.1, k1
asig1    poscil.2, k1 * 500 + 100
asig2    poscil.2, k1 * 500 + 200
outs    asig1, asig2    
endin

</CsInstruments>
<CsScore>
i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
