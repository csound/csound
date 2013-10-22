<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o lphashor.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>
; Example by Jonathan Murphy Dec 2006

  sr	    =  44100
  ksmps	    =  10
  nchnls    =  1

    instr 1

  ifn	    =  1   ; table number
  ilen	    =  nsamp(ifn)    ; return actual number of samples in table
  itrns	    =  1   ; no transposition
  ilps	    =  0   ; loop starts at index 0
  ilpe	    =  ilen ; ends at value returned by nsamp above
  imode	    =  3    ; loop forwards & backwards
  istrt	    =  10000  ; commence playback at index 10000 samples
  ; lphasor provides index into f1 
  alphs	    lphasor   itrns, ilps, ilpe, imode, istrt
  atab	    tablei    alphs, ifn
	    ; amplify signal
  atab	    =  atab * 10000

	    out	      atab

    endin

</CsInstruments>

<CsScore>
f 1 0 262144 1 "beats.wav" 0 4 1
i1 0 60
e
</CsScore>

</CsoundSynthesizer>