<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o convolve.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; NB: 'Small' reverbs often require a much higher percentage of wet signal to sound interesting. 'Large'
; reverbs seem require less. Experiment! The wet/dry mix is very important - a small change can make a large difference.

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; by Menno Knevel - 2021

ires1 system_i 1,{{ cvanal rv_mono.wav rv_mono.con }}  ; analyze mono spring reverb
ires2 system_i 1,{{ cvanal rv_stereo.wav rv_stereo.con }} ; analyze stereo spring reverb

instr 1 

imix = 0.25	;wet/dry mix. Vary as desired.
ivol = .8 	;Overall volume level of reverb. Adjust to avoid clipping.

idel   filelen p4			;calculate length and number of channels of soundfile
ichnls filenchnls  p4
prints	"\n**this reverb file = %f seconds and has %d channel(s)**\n", idel, ichnls

if (ichnls == 1) then					; if mono
	adry    soundin "fox.wav"               ; input (dry) audio
	awet    convolve adry,"rv_mono.con"	; mono convolved (wet) audio
	awet1   diff    awet                    ; brighten sound
	awet2	=	awet1						; as it is a mono file played stereo
	adrydel delay   (1-imix)*adry, idel	; Delay dry signal to align it with convolved signal
else									; if stereo
	adry    soundin "fox.wav"               ; input (dry) audio
	awet1, awet2 convolve adry,"rv_stereo.con" ; stereo convolved (wet) audio
	awet1   diff    awet1                   ; brighten left sound
	awet2   diff    awet2                   ; and brighten right sound
	adrydel delay   (1-imix)*adry, idel     ; Delay dry signal to align it with convolved signal
endif
outs    ivol*(adrydel+imix*awet1),ivol*(adrydel+imix*awet2) ; Mix wet & dry signals

endin

</CsInstruments>
<CsScore>

i 1 0 4 "rv_mono.wav"
i 1 5 4 "rv_stereo.wav"
e
</CsScore>
</CsoundSynthesizer>
