<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

;test global aArrays
;jh march 2013 (using code from iain mccurdy)

sr = 44100
ksmps = 32
nchnls = 2

gaArr[]    init       2

  instr 1 ; left channel
kEnv       loopseg    0.5, 0, 0, 1,0.003, 1,0.0001, 0,0.9969
aSig       pinkish    kEnv 
gaArr[0]   =          aSig * 20000
  endin

  instr 2 ; right channel
kEnv       loopseg    0.5, 0, 0.5, 1,0.003, 1,0.0001, 0,0.9969
aSig       pinkish    kEnv
gaArr[1]   =          aSig * 20000
  endin

  instr 3 ; reverb
aInSigL    =          gaArr[0] / 3
aInSigR    =          gaArr[1] / 2
aRvbL,aRvbR reverbsc  aInSigL, aInSigR, 0.88, 8000
gaArr[0]   =          gaArr[0] + aRvbL
gaArr[1]   =          gaArr[1] + aRvbR
           outs       gaArr[0], gaArr[1]
gaArr[0]   =          0
gaArr[1]   =          0
  endin
</CsInstruments>
<CsScore>
i 1 0 10
i 2 0 10
i 3 0 12
</CsScore>
</CsoundSynthesizer>

