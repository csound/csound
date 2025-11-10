<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Test comprehensive global variable scenarios
giInt = 42
gkRate init 100
gaAudio init 0
gSString = "hello"
giArray[] fillarray 1, 2, 3, 4, 5

instr 1
  ; Test i-rate global
  print giInt
  
  ; Test k-rate global
  printk 0.1, gkRate
  
  ; Test string global
  puts gSString, 1
  
  ; Test array global
  print giArray[0]
  print giArray[4]
  
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
