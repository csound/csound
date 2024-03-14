<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac    -m0d --midi-key-oct=4 --midi-velocity=5   -F midiChords.mid 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pset-midi.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

; by Menno Knevel - 2021

sr = 44100 
ksmps = 32
0dbfs  = 1 
nchnls = 2 

; midiChords.mid can be found in examples folder

instr 1

            pset 1, 0, .1

istarttime  = p2
iattack     = 0.005
isustain    = p3
irelease    = 0.06
p3          = isustain + iattack + irelease

ifrequency cpsmidi
iamplitude  = p5*.2			;lower volume

print p1, p2, p3, p4, p5
asig STKBandedWG ifrequency, iamplitude
     outs asig, asig

endin
</CsInstruments>
<CsScore>
i1 0 60  ; runs for 1 minute, midifile time lasts for 35 seconds
</CsScore>
</CsoundSynthesizer>
