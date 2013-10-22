<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
;-odac     ;;;RT audio out
-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o convolve.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; NB: 'Small' reverbs often require a much higher
; percentage of wet signal to sound interesting. 'Large'
; reverbs seem require less. Experiment! The wet/dry mix is
; very important - a small change can make a large difference.

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1
;The analysis file is not system independent! 
; create "rv_mono.wav" and "rv_stereo.wav" with cvanal first!

instr 1 

imix = 0.25	;wet/dry mix. Vary as desired.
ivol = 1 	;Overall volume level of reverb. May need to adjust
;when wet/dry mix is changed, to avoid clipping.

idel   filelen p4			;calculate length and number of channels of soundfile
print  idel
ichnls filenchnls  p4
print  ichnls

if (ichnls == 1) then

adry    soundin "fox.wav"               ; input (dry) audio
awet    convolve adry,"rv_mono.cva"	; mono convolved (wet) audio
awet    diff    awet                    ; brighten
adrydel delay   (1-imix)*adry, idel	; Delay dry signal to align it with convolved signal
					; Apply level adjustment here too.
outs    ivol*(adrydel+imix*awet),ivol*(adrydel+imix*awet) ; Mix wet & dry

else

adry    soundin "fox.wav"               ; input (dry) audio
awet1, awet2 convolve adry,"rv_stereo.cva" ; stereo convolved (wet) audio
awet1   diff    awet1                   ; brighten left
awet2   diff    awet2                   ; and brighten right
adrydel delay   (1-imix)*adry, idel     ; Delay dry signal to align it with convolved signal
					; Apply level adjustment here too.
outs    ivol*(adrydel+imix*awet1),ivol*(adrydel+imix*awet2) ; Mix wet & dry signals

endif

endin

</CsInstruments>
<CsScore>

i 1 0 4 "rv_mono.wav"
i 1 5 4 "rv_stereo.wav"

e
</CsScore>
</CsoundSynthesizer>
