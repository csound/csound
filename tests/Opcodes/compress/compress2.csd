<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -iadc    ;;;RT audio out and in
; For Non-realtime ouput leave only the line below:
; -o compress2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
nchnls_i = 1    ; assume only one mono signal for audio input!
0dbfs  = 1

instr 1	; uncompressed signal

asig diskin2 "drumsMlp.wav", 1, 0, 1
     outs asig, asig
endin


instr 2	; compressed signal, use the "drumsMlp.wav" audio file and a soundfile

avoice  diskin2 "Mathews.wav", 1, 0, 1
asig   diskin2 "drumsMlp.wav", 1, 0, 1
prints "***compressing according to Mathews.wav***\n\n"
  kthresh = -90
  kloknee = -50
  khiknee = -30
  kratio  = 6
  katt    = 0.01
  krel    = .1
  ilook   = .02
asig  compress2 asig, avoice, kthresh, kloknee, khiknee, kratio, katt, krel, ilook	; voice-activated compressor
      outs asig, asig
endin

instr 3	; compressed signal, use the "drumsMlp.wav" audio file and a mic

avoice in                                     ; duck the audio signal "drumsMlp.wav" with your voice.
asig   diskin2 "drumsMlp.wav", 1, 0, 1
prints "***compressing according to your mic -- if present***\n\n"
  kthresh = -90
  kloknee = -50
  khiknee = -30
  kratio  = 5
  katt    = 0.1
  krel    = .5
  ilook   = .02
asig  compress2 asig, avoice, kthresh, kloknee, khiknee, kratio, katt, krel, ilook	; voice-activated compressor
      outs asig, asig
endin

</CsInstruments>
<CsScore>
s
i1 0 4
s
i2 0 16
s
i3 0 30

e
</CsScore>
</CsoundSynthesizer>
