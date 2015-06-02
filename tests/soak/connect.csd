<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o connect.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Michael Gogins */
; Initialize the global variables.
sr = 44100
ksmps = 32
nchnls = 2

; Connect up the instruments to create a signal flow graph.

connect "SimpleSine",   "leftout",     "Reverberator",     	"leftin"
connect "SimpleSine",   "rightout",    "Reverberator",     	"rightin"

connect "Moogy",        "leftout",     "Reverberator",     	"leftin"
connect "Moogy",        "rightout",    "Reverberator",     	"rightin"

connect "Reverberator", "leftout",     "Compressor",       	"leftin"
connect "Reverberator", "rightout",    "Compressor",       	"rightin"

connect "Compressor",   "leftout",     "Soundfile",       	"leftin"
connect "Compressor",   "rightout",    "Soundfile",       	"rightin"

; Turn on the "effect" units in the signal flow graph.

alwayson "Reverberator", 0.91, 12000
alwayson "Compressor"
alwayson "Soundfile"

instr SimpleSine
  ihz = cpsmidinn(p4)
  iamplitude = ampdb(p5)
  print ihz, iamplitude
  ; Use ftgenonce instead of ftgen, ftgentmp, or f statement.
  isine ftgenonce 0, 0, 4096, 10, 1
  a1 oscili iamplitude, ihz, isine
  aenv madsr 0.05, 0.1, 0.5, 0.2
  asignal = a1 * aenv
  ; Stereo audio outlet to be routed in the orchestra header.
  outleta "leftout", asignal * 0.25
  outleta "rightout", asignal * 0.75
endin

instr Moogy
  ihz = cpsmidinn(p4)
  iamplitude = ampdb(p5)
  ; Use ftgenonce instead of ftgen, ftgentmp, or f statement.
  isine ftgenonce 0, 0, 4096, 10, 1
  asignal vco iamplitude, ihz, 1, 0.5, isine
  kfco line 200, p3, 2000
  krez init 0.9
  asignal moogvcf asignal, kfco, krez, 100000
  ; Stereo audio outlet to be routed in the orchestra header.
  outleta "leftout", asignal * 0.75
  outleta "rightout", asignal * 0.25
endin

instr Reverberator
  ; Stereo input.
  aleftin inleta "leftin"
  arightin inleta "rightin"
  idelay = p4
  icutoff = p5
  aleftout, arightout reverbsc aleftin, arightin, idelay, icutoff
  ; Stereo output.
  outleta "leftout", aleftout
  outleta "rightout", arightout 
endin

instr Compressor
  ; Stereo input.
  aleftin inleta "leftin"
  arightin inleta "rightin"
  kthreshold = 25000
  icomp1 = 0.5
  icomp2 = 0.763
  irtime = 0.1
  iftime = 0.1
  aleftout dam aleftin, kthreshold, icomp1, icomp2, irtime, iftime
  arightout dam arightin, kthreshold, icomp1, icomp2, irtime, iftime
  ; Stereo output.
  outleta "leftout", aleftout 
  outleta "rightout", arightout 
endin

instr Soundfile
  ; Stereo input.
  aleftin inleta "leftin"
  arightin inleta "rightin"
  outs aleftin, arightin
endin

</CsInstruments>
<CsScore>
; Not necessary to activate "effects" or create f-tables in the score!
; Overlapping notes to create new instances of instruments.
i "SimpleSine" 1 5 60 85
i "SimpleSine" 2 5 64 80
i "Moogy" 3 5 67 75
i "Moogy" 4 5 71 70
;6 extra seconds after the performance
e 12
</CsScore>
</CsoundSynthesizer>