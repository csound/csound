<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvscent.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;; example written by joachim heintz 2009

giSine		ftgen		0, 0, 4096, 10, 1

instr 1
irefrtm     = p4; time for generating new values for the spectral centroid
ifftsize	= 1024
ioverlap	= ifftsize / 4
iwinsize	= ifftsize
iwinshape	= 1; von-Hann window
;Sfile		= "flute-C-octave0.wav"
Sfile		= "fox.wav"
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape; fft-analysis of the audio-signal
ktrig		metro	1 / irefrtm
if ktrig == 1 then
kcenter	pvscent	fftin; spectral center
endif
aout		oscil	.2, kcenter, giSine
outs		aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0 2.757 .3
i 1 3 2.757 .05
i 1 6 2.757 .005
i 1 9 2.757 .001
e
</CsScore>
</CsoundSynthesizer>
