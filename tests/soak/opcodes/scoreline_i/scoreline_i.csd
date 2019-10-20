<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

scoreline_i {{
            i 2  0  3  "flute.aiff"
            i 2  1  3  "beats.wav"
            }}
        
endin

instr 2

asig soundin p4
     outs asig*.8, asig*.8

endin
</CsInstruments>
<CsScore>

i1 0 1
e
</CsScore>
</CsoundSynthesizer>
