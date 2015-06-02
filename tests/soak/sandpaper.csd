<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sandpaper.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idmp = p4
a1   line 2, p3, 2			;preset amplitude increase
a2   sandpaper 1, 0.01, 128, idmp	;sandpaper needs a little amp help at these settings
asig product a1, a2			;increase amplitude
     outs asig, asig
          
endin
</CsInstruments>
<CsScore>
i1 0 1 0.5
i1 + 1 0.95

e
</CsScore>
</CsoundSynthesizer>
