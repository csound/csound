<CsoundSynthesizer>

<CsInstruments>
sr=44100
ksmps=1
nchnls=2

giamp = 10000

	instr 1	;untitled

iamp = giamp


kfreq init p4

kenv	linseg 0, p3 * (.25 + .25), 1, p3 * .5 * 1, 0
kenv 	= kenv * iamp

if (p5 == 0) then
    if (p6 == 0) then
        kfreq = p4 * .5
    elseif (p6 == 1) then
        kfreq = p4 * .75
    else  
        kfreq = p4 * .9
    endif	
	
elseif (p5 ==1) then
    if (p6 == 0) then
        kfreq = kfreq + .0002
    elseif (p6 == 1) then
        kfreq = kfreq + .0006
    else  
        kfreq = kfreq - .0007
    endif   
else
    kfreq = p4 * 2
endif

aout	vco2 kenv, kfreq

	outs aout, aout
	endin


</CsInstruments>

<CsScore>

i1	0.0	2 440 0 0
i1  + . 440 0 1
i1  + . 440 0 2
i1  + . 440 1 0
i1  + . 440 1 1
i1  + . 440 1 2
i1  + . 440 2 0
e

</CsScore>

</CsoundSynthesizer>
