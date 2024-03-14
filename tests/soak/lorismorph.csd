<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lorismorph.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
; clarinet.sdif and meow.sdif can be found in /manual/examples

; Morph the partials in meow.sdif into the partials in clarinet.sdif over the duration of
; the sustained portion of the two tones (from .2 to 2.0 seconds in the meow, and from
; .5 to 2.1 seconds in the clarinet). The onset and decay portions in the morphed sound are
; specified by parameters p4 and p5, respectively. The morphing time is the time between the
; onset and the decay. The meow partials are shfited in pitch to match the pitch of the clarinet
; tone (D above middle C). 

instr 1

ionset	= p4
idecay	= p5
itmorph = p3 - (ionset + idecay)
ipshift	= cpspch(8.02)/cpspch(8.08)
    
ktme	linseg	0, ionset, .2, itmorph, 2.0, idecay, 2.1    ; meow time function, morph from .2 to 2.0 seconds
ktcl	linseg	0, ionset, .5, itmorph, 2.1, idecay, 2.3    ; clarinet time function, morph from .5 to 2.1 seconds
kmurph	linseg	0, ionset, 0, itmorph, 1, idecay, 1
	lorisread  ktme, "meow.sdif", 1, ipshift, 2, 1, .001
	lorisread  ktcl, "clarinet.sdif", 2, 1, 1, 1, .001
	lorismorph 1, 2, 3, kmurph, kmurph, kmurph
asig	lorisplay  3, 1, 1, 1
	outs	   asig, asig
endin

; Morph the partials in clarinet.sdif into the partials in meow.sdif. The start and end times
; for the morph are specified by parameters p4 and p5, respectively. The morph occurs over the
; second of four pitches in each of the sounds, from .75 to 1.2 seconds in the flutter-tongued
; clarinet tone, and from 1.7 to 2.2 seconds in the cat's meow. Different morphing functions are
; used for the frequency and amplitude envelopes, so that the partial amplitudes make a faster  
; transition from clarinet to cat than the frequencies. (The bandwidth envelopes use the same          
; morphing function as the amplitudes.) 

instr 2

ionset	 = p4
imorph	 = p5 - p4
irelease = p3 - p5
    
ktclar	linseg	0, ionset, .75, imorph, 1.2, irelease, 2.4
ktmeow	linseg	0, ionset, 1.7, imorph, 2.2, irelease, 3.4
    
kmfreq	linseg	0, ionset, 0, .75*imorph, .25, .25*imorph, 1, irelease, 1
kmamp	linseg	0, ionset, 0, .75*imorph, .9, .25*imorph, 1, irelease, 1
    
	lorisread  ktclar, "clarinet.sdif", 1, 1, 1, 1, .001
	lorisread  ktmeow, "meow.sdif", 2, 1, 1, 1, .001
	lorismorph 1, 2, 3, kmfreq, kmamp, kmamp
asig	lorisplay  3, 1, 1, 1
	outs	   asig, asig
endin

</CsInstruments>   
<CsScore>
;     strt   dur   onset   decay
i 1    0      3     .25     .15
i 1    +      1     .10     .10
i 1    +      6    1.      1.

;    strt   dur  morph_start   morph_end
 i 2    9    4     .75           2.75

e
</CsScore>  
</CsoundSynthesizer>
