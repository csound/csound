<CsoundSynthesizer>

<CsOptions>
  -d -o dac
</CsOptions>

<CsInstruments>
sr        =         96000
ksmps     =         10
nchnls    =         2
0dbfs     =         1

FLpanel "crossfmForm", 600, 400, 0, 0
  gkfrq1, ihfrq1 FLcount "Freq #1", 0, 20000, 0.001, 1, 1, 200, 30, 20, 50, -1
  gkfrq2, ihfrq2 FLcount "Freq #2", 0, 20000, 0.001, 1, 1, 200, 30, 20, 130, -1
  gkndx1, gkndx2, ihndx1, ihndx2 FLjoy "Indexes", 0, 10, 0, 10, 0, 0, -1, -1, 200, 200, 300, 50
  
  FLsetVal_i 164.5, ihfrq1
  FLsetVal_i 263.712, ihfrq2
  FLsetVal_i 1.5, ihndx1
  FLsetVal_i 3, ihndx2
FLpanelEnd
FLrun

maxalloc 1, 2

          instr 1
kamp      linen     0.5, 0.01, p3, 0.5 
a1,a2     crossfm   gkfrq1, gkfrq2, gkndx1, gkndx2, 1, 1, 1
          outs      a1*kamp, a2*kamp
          endin
</CsInstruments>

<CsScore>
f1 0 16384 10 1 0
i1 0 60
e
</CsScore>
</CsoundSynthesizer>

