<CsoundSynthesizer>
<CsOptions>
-dnm0
</CsOptions>
<CsInstruments>

;test local SArrays 
;(same code in instr 1 and 2, different values)
;create and fill string array at i-time, modify at k-time
;jh march 2013

  opcode StrAgrm, S, Sj
  ;changes the elements in Sin randomly, like in an anagram
Sin, iLen  xin
 if iLen == -1 then
iLen       strlen     Sin
 endif
Sout       =          ""
;for all elements in Sin
iCnt       =          0
iRange     =          iLen
loop:
;get one randomly
iRnd       rnd31      iRange-.0001, 0
iRnd       =          int(abs(iRnd))
Sel        strsub     Sin, iRnd, iRnd+1
Sout       strcat     Sout, Sel
;take it out from Sin
Ssub1      strsub     Sin, 0, iRnd
Ssub2      strsub     Sin, iRnd+1
Sin        strcat     Ssub1, Ssub2
;adapt range (new length)
iRange     =          iRange-1
           loop_lt    iCnt, 1, iLen, loop
           xout       Sout
  endop


  instr 1
           prints     "SArr[] in instr %d at init-time:\n  [", p1
;create
S_Arr[]    init       4
;fill
iCounter   =          0
           until      (iCounter == 4) do
S_new      StrAgrm    "csound"
S_Arr[iCounter] =     S_new
iCounter += 1
od
;print
iCounter   =          0
           until      (iCounter == 4) do
           printf_i   "%s ", iCounter+1, S_Arr[iCounter]
iCounter += 1
od
           prints     "]\n"

kCycle     timeinstk
           printks    "SArr[] in instr %d at k-cycle %d:\n  [", 0, p1, kCycle
;fill
kCounter   =          0
           until      (kCounter == 4) do
kChar      random     33, 127
S_new      sprintfk   "%c ", int(kChar)
S_Arr[kCounter] strcpyk S_new ;'=' should work but does not
kCounter += 1
od
;print
kCounter   =          0
           until      (kCounter == 4) do
           printf     "%s ", kCounter+1, S_Arr[kCounter]
kCounter += 1
od
           printks    "]\n", 0
if kCycle == 3 then
           turnoff
endif
  endin

  instr 2
           prints     "SArr[] in instr %d at init-time:\n  [", p1
;create
S_Arr[]    init       4
;fill
iCounter   =          0
           until      (iCounter == 4) do
S_new      StrAgrm    "csound"
S_Arr[iCounter] =     S_new
iCounter += 1
od
;print
iCounter   =          0
           until      (iCounter == 4) do
           printf_i   "%s ", iCounter+1, S_Arr[iCounter]
iCounter += 1
od
           prints     "]\n"

kCycle     timeinstk
           printks    "SArr[] in instr %d at k-cycle %d:\n  [", 0, p1, kCycle
;fill
kCounter   =          0
           until      (kCounter == 4) do
kChar      random     33, 127
S_new      sprintfk   "%c ", int(kChar)
S_Arr[kCounter] strcpyk S_new ;'=' should work but does not
kCounter += 1
od
;print
kCounter   =          0
           until      (kCounter == 4) do
           printf     "%s ", kCounter+1, S_Arr[kCounter]
kCounter += 1
od
           printks    "]\n", 0
if kCycle == 3 then
           turnoff
endif
  endin

</CsInstruments>    
<CsScore>
i 1 0 1
i 2 1 1
</CsScore>
</CsoundSynthesizer>
Prints:
SArr[] in instr 1 at init-time:
  [consdu uncdos oduscn scodun ]
SArr[] in instr 1 at k-cycle 1:
  [s  <  x  +  ]
SArr[] in instr 1 at k-cycle 2:
  [!  P  Z  r  ]
SArr[] in instr 1 at k-cycle 3:
  [u  U  b  K  ]
SArr[] in instr 2 at init-time:
  [uocdsn odscnu uscdon sncduo ]
SArr[] in instr 2 at k-cycle 1:
  [c  "  h  h  ]
SArr[] in instr 2 at k-cycle 2:
  [9  =  #  U  ]
SArr[] in instr 2 at k-cycle 3:
  [x  F  Z  l  ]

<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<MacGUI>
ioView nobackground {65535, 65535, 65535}
</MacGUI>
