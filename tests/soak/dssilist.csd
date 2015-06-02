<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out  
-odac  

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1	; list all DSSI and LADSPA plugins

dssilist

endin
</CsInstruments>
<CsScore>
i 1 0 0

e
</CsScore>
</CsoundSynthesizer>
