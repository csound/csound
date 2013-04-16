<CsoundSynthesizer>
<CsOptions>
-dnm0
</CsOptions>
<CsInstruments>

;test global SArrays
;jh march 2013

giArrLen  =        5
gSArr[]   init     giArrLen

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
           prints     "Filling gSArr[] in instr %d at init-time!\n", p1
iCounter   =          0
           until      (iCounter == giArrLen) do
S_new      StrAgrm    "csound"
gSArr[iCounter] =     S_new
iCounter += 1
od
endin

instr 2
           prints     "Printing gSArr[] in instr %d at init-time:\n  [", p1
iCounter   =          0
           until      (iCounter == giArrLen) do
           printf_i   "%s ", iCounter+1, gSArr[iCounter]
iCounter += 1
od
           prints     "]\n"
endin

instr 3
          printks   "Printing gSArr[] in instr %d at perf-time:\n  [", 0, p1
kcounter  =        0
  until (kcounter == giArrLen) do
          printf   "%s ", kcounter+1, gSArr[kcounter]
kcounter  +=       1
  od
          printks  "]\n", 0
          turnoff
endin

instr 4
           prints     "Modifying gSArr[] in instr %d at init-time!\n", p1
iCounter   =          0
           until      (iCounter == giArrLen) do
S_new      StrAgrm    "csound"
gSArr[iCounter] =     S_new
iCounter += 1
od
endin

instr 5
           prints     "Printing gSArr[] in instr %d at init-time:\n  [", p1
iCounter   =          0
           until      (iCounter == giArrLen) do
           printf_i   "%s ", iCounter+1, gSArr[iCounter]
iCounter += 1
od
           prints     "]\n"
endin

instr 6
kCycle     timeinstk
           printks    "Modifying gSArr[] in instr %d at k-cycle %d!\n", 0, p1, kCycle
kCounter   =          0
           until      (kCounter == giArrLen) do
kChar      random     33, 127
S_new      sprintfk   "%c ", int(kChar)
gSArr[kCounter] strcpyk S_new ;'=' should work but does not
kCounter += 1
od
if kCycle == 3 then
           turnoff
endif
endin

instr 7
kCycle     timeinstk
           printks    "Printing gSArr[] in instr %d at k-cycle %d:\n  [", 0, p1, kCycle
kCounter   =          0
           until      (kCounter == giArrLen) do
           printf     "%s ", kCounter+1, gSArr[kCounter]
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
i 2 0 1
i 3 0 1
i 4 1 1
i 5 1 1
i 6 1 1
i 7 1 1
</CsScore>
</CsoundSynthesizer>
prints:
Filling gSArr[] in instr 1 at init-time!
Printing gSArr[] in instr 2 at init-time:
  [nudosc coudns dsocun ocsund osncdu ]
Printing gSArr[] in instr 3 at perf-time:
  [nudosc coudns dsocun ocsund osncdu ]
Modifying gSArr[] in instr 4 at init-time!
Printing gSArr[] in instr 5 at init-time:
  [ousndc uocdns sudocn usnocd ouncds ]
Modifying gSArr[] in instr 6 at k-cycle 1!
Printing gSArr[] in instr 7 at k-cycle 1:
  [s  <  x  +  !  ]
Modifying gSArr[] in instr 6 at k-cycle 2!
Printing gSArr[] in instr 7 at k-cycle 2:
  [P  Z  r  u  U  ]
Modifying gSArr[] in instr 6 at k-cycle 3!
Printing gSArr[] in instr 7 at k-cycle 3:
  [b  K  c  "  h  ]

