<CsoundSynthesizer> 
 
<CsOptions> 
-+rtaudio=JACK -b 64 --sched  -o dac:system:playback_ 
</CsOptions> 
 
<CsInstruments> 
sr      =      	44100
ksmps  	=     	16
nchnls	=	2


	instr 1
jacktransport p4, p5 
	endin

	instr 2
jacktransport p4 
	endin


</CsInstruments> 
<CsScore>

i2 0 5 1; play
i2 5 1 0; stop
i1 6 5 1 2 ; move at 2 seconds and start playing back
i1 11 1 0 0 ; stop and rewind

e

</CsScore> 
 </CsoundSynthesizer>
