<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

asnd init 0 			;input of fin must be initialized
     fin  "fox.wav", 0, 0, asnd	;read audiofile
aenv follow asnd, 0.01		;envelope follower
kenv downsamp aenv
asig rand kenv			;gate the noise with audiofile
     outs asig, asig 
endin 

</CsInstruments>
<CsScore>

i 1 0 3
e

</CsScore>
</CsoundSynthesizer>
