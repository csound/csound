<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d         -M0  -+rtmidi=virtual ;;;RT audio I/O with MIDI in
; For Non-realtime ouput leave only the line below:
; -o midiin.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>

sr        =  44100
ksmps     =  10
nchnls    =  1

; Example by schwaahed 2006

      massign         0, 130  ; make sure that all channels
      pgmassign       0, 130  ; and programs are assigned to test instr


  instr   130

knotelength    init    0
knoteontime    init    0

kstatus, kchan, kdata1, kdata2                  midiin

if (kstatus == 128) then
knoteofftime    times
knotelength    =    knoteofftime - knoteontime
printks "kstatus= %d, kchan = %d, \\tnote#  = %d, velocity = %d \\tNote OFF\\t%f %f\\n", 0, kstatus, kchan, kdata1,kdata2, knoteofftime, knotelength

elseif (kstatus == 144) then
knoteontime    times
printks "kstatus= %d, kchan = %d, \\tnote#  = %d, velocity = %d \\tNote ON\\t%f\\n", 0, kstatus, kchan, kdata1, kdata2, knoteontime


elseif (kstatus == 160) then
printks "kstatus= %d, kchan = %d, \\tkdata1 = %d, kdata2 = %d \\tPolyphonic Aftertouch\\n", 0, kstatus, kchan, kdata1, kdata2

elseif (kstatus == 176) then
printks "kstatus= %d, kchan = %d, \\t CC = %d, value = %d \\tControl Change\\n", 0, kstatus, kchan, kdata1, kdata2

elseif (kstatus == 192) then
printks "kstatus= %d, kchan = %d, \\tkdata1 = %d, kdata2 = %d \\tProgram Change\\n", 0, kstatus, kchan, kdata1, kdata2

elseif (kstatus == 208) then
printks  "kstatus= %d, kchan = %d, \\tkdata1 = %d, kdata2 = %d \\tChannel Aftertouch\\n", 0, kstatus, kchan, kdata1, kdata2

elseif (kstatus == 224) then
printks "kstatus= %d, kchan = %d, \\t ( data1 , kdata2 ) = ( %d, %d )\\tPitch Bend\\n", 0, kstatus, kchan, kdata1, kdata2

endif

  endin

</CsInstruments>

<CsScore>
i130 0 3600
e
</CsScore>

</CsoundSynthesizer> 