<CsoundSynthesizer>
<CsOptions>
;;-b100 -B100 -odac3
</CsOptions>
<CsInstruments>
sr=44100
kr=441
ksmps=100
nchnls=2

        FLpanel "This Panel contains a Knob and shows its current value",400,300
ih1     FLvalue "This is current value of output of the Knob", 100,20,50,20
gk1,gih  FLknob  "Current value of this Knob is shown in a text field", 80,5000,-1,1, ih1, 90, 150,100
        FLpanelEnd
        FLrun

        instr 1
        FLsetVal_i 440, gih
a1      oscili  3000, gk1, 1
        outs     a1,a1
        endin
</CsInstruments>
<CsScore>
f1 0 1024 10 1
i1 0 3600


</CsScore>
</CsoundSynthesizer>
