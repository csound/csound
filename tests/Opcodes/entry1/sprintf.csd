<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sprintf.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;the file "impuls20.aiff" can be found in /manual/examples
instr 1

ifn = 20
Sname sprintf "impuls%02d.aiff", ifn
Smsg  sprintf "The file name is: '%s'", Sname
      puts    Smsg, 1
asig  soundin Sname
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
