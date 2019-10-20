<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 ; choose between mono or stereo file

ichn filenchnls  p4	;check number of channels
print  ichn

if ichn == 1 then	
asig   soundin p4	;mono signal
       outs    asig, asig
else			;stereo signal
aL, aR soundin p4
       outs    aL, aR
endif

endin
</CsInstruments>
<CsScore>

i 1 0 3 "fox.wav"	;mono signal
i 1 5 2 "kickroll.wav"	;stereo signal

e
</CsScore>
</CsoundSynthesizer>
