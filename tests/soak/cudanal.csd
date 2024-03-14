<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cudanal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	;cudanal has no influence when there is no transformation of original sound

ifftsize  = p4
ioverlap  = ifftsize / 4
iwinsize  = ifftsize
iwinshape = 1							;von-Hann window
Sfile     = "fox.wav"
ain       soundin Sfile
fftin     cudanal ain, ifftsize, ioverlap, iwinsize, iwinshape	;fft-analysis of the audio-signal
fftblur   pvscale fftin, p5					;scale
aout      cudasynth fftblur					;resynthesis
          outs	  aout, aout
endin

</CsInstruments>
<CsScore>
s
i 1 0 3 512 1		;original sound - ifftsize of pvsanal does not have any influence
i 1 3 3 1024 1		;even with different
i 1 6 3 2048 1		;settings

s
i 1 0 3 512 1.5		;but transformation - here a fifth higher
i 1 3 3 1024 1.5	;but with different settings
i 1 6 3 2048 1.5	;for ifftsize of pvsanal

e
</CsScore>
</CsoundSynthesizer>

