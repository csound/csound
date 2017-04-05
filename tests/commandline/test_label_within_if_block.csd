<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
iToggle = 1

if (iToggle == 0) then
    iNumberChildren = 5
    SampleSetChildrenLoop:
        print iNumberChildren
        iNumberChildren = iNumberChildren - 1
    if (iNumberChildren > 0.) goto SampleSetChildrenLoop 
else    
    iLop = 5
    Looper:
        print iLop
        iLop = iLop - 1
    if (iLop > 0.) goto Looper 
endif


endin
</CsInstruments>
<CsScore>

i 1 0 1
e
</CsScore>
</CsoundSynthesizer>

