<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tableikt.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ilowfn	= p4					;lowest ftable wave
ihighfn	= p5					;highest ftable wave

kswpenv	line 1, p3, 0				;sweep envelope, calculate current table pair and interpolation amount
inumtables = ihighfn - ilowfn			;1 less than number of tables
kfn1	= int(kswpenv*inumtables) + ilowfn
        printks "play table no: %d\n", 1, kfn1
kfn2	= kfn1 + 1
kinterp	= frac(kswpenv*inumtables)		
ixmode  = 1					;read tables with phasor
aphase	phasor	40	
asig	tableikt aphase, kfn1, ixmode		;normalized index
   if kswpenv == 1.0 kgoto skipfn2		;if kfn1 is last table, there is no kfn2
	asig2	tableikt aphase, kfn2, ixmode
   skipfn2:
   amix	ntrpol	asig, asig2, kinterp		;interpolate between tables and output
	outs	amix*.5, amix*.5
	
endin
</CsInstruments>
<CsScore>
f 1  0 16384 10 1  
f 2  0 16384 10 1 .5  
f 3  0 16384 1 "fox.wav" 0 0 0				;a sample
f 4  0 16384 10 1 .5 .3 .25 .2 .16 .14 .125 .111	;sawtooth 
f 5  0 16384 10 1 .4 .3 .25 .2
f 6  0 16384 10 1 .3 .3 .25 .2 .16 
f 7  0 16384 10 1  1  1  1  .7 .5  .3  .1		;pulse
f 8  0 16384 1 "beats.wav" 0 0 0			;a sample

i 1 0 10 1 8
e
</CsScore>
</CsoundSynthesizer>

