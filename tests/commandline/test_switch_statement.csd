<CsoundSynthesizer>
<CsInstruments>

// switch in UDO must not break xout typing or branch logic
opcode switch_xout_value, i, i
  iCond xin
  iOut = -1
  switch iCond
    case 1
      iOut = 11
    case 2
      iOut = 22
    case 3
      iOut = 33
    default
      iOut = 44
  endsw
  xout iOut
endop

// empty switch cases in UDO must not crash and should not fall through to default
opcode switch_empty_case_udo, i, i
  iCond xin
  iDefault = 0
  switch iCond
    case 1
    case 2
    default
      iDefault = 1
  endsw
  xout iDefault
endop

// accepts p-fields
instr 1
  switch p4
    case 1
      prints "pass\n"
    default
      prints "test1 fail\n"
      exitnow(-1)
  endsw
  prints "test1 passed\n"
endin

// accepts expression
instr 2
  switch 1 + 1
    case 2
      prints "pass\n"
    default
     prints "fail\n"
     exitnow(-1)
  endsw
  prints "test2 passed\n"
endin

// accepts DRY multi match cases
instr 3
  switch 3
    case 0, 2
      prints "test3 failed\n"
      exitnow(-1)
    case 1, 3
      prints "pass\n"
    default
     prints "test3 fail\n"
     exitnow(-1)
  endsw
  prints "test3 passed\n"
endin

// accepts expression in case
instr 4
  switch 3
    case 1 + 2
      prints "pass\n"
    default
      prints "test4 fail\n"
      exitnow(-1)
  endsw
  prints "test4 passed\n"
endin

// operates on performance rate
instr 5
  kl = line(0, p3 - 1/kr, 1)
  switch int(kl)
    case 1
      printks2 "pass %d\n", kl
  endsw
  prints "test5 passed\n"
endin

// cases can be combined
instr 6
  iAssertCase = 0
  iAssertDefault = 0
  switch p4
    case 1
    case 2
      prints "1 or 2\n"
      iAssertCase = 1
    default
      prints "Default\n"
      iAssertDefault = 1
  endsw
  if iAssertCase == 0 && p4 < 3 then
    prints "assert error, expected 1 or 2\n"
    exitnow(-1)
  endif
  if iAssertDefault == 0 && p4 >= 3 then
    prints "assert error, default was expected but did not execute\n"
    exitnow(-1)
  endif
  prints "test6 passed\n"
endin

// cases without statement are correctly ignored
instr 7
  iAssertDefault = 0
  switch p4
    case 1
    case 2
    default
      prints "Default, val: %d\n", p4
      iAssertDefault = 1
  endsw
  if iAssertDefault == 0 && p4 == 3 then
    prints "assert error, default not executed when expected\n"
    exitnow(-1)
  endif
  if iAssertDefault == 1 && p4 < 3 then
    prints "assert error, fell back to default when not expected\n"
    exitnow(-1)
  endif
  prints "test7 passed\n"
endin

// empty default body is accepted and does not alter control flow
instr 8
  iAssertCase = 0
  iAfterSwitch = 0
  switch p4
    case 1
      iAssertCase = 1
    default
  endsw
  iAfterSwitch = 1
  if p4 == 1 && iAssertCase == 0 then
    prints "assert error, case 1 did not execute\n"
    exitnow(-1)
  endif
  if p4 != 1 && iAssertCase != 0 then
    prints "assert error, case 1 executed unexpectedly\n"
    exitnow(-1)
  endif
  if iAfterSwitch == 0 then
    prints "assert error, control did not continue after empty default\n"
    exitnow(-1)
  endif
  prints "test8 passed\n"
endin

// no-default switches do nothing on no-match and continue
instr 9
  iAssertCase = 0
  switch p4
    case 1
      iAssertCase = 1
  endsw
  if p4 == 1 && iAssertCase == 0 then
    prints "assert error, no-default case did not match\n"
    exitnow(-1)
  endif
  if p4 != 1 && iAssertCase != 0 then
    prints "assert error, no-default switch matched unexpectedly\n"
    exitnow(-1)
  endif
  prints "test9 passed\n"
endin

// trailing empty case without default is ignored
instr 10
  iAssertCase = 0
  switch p4
    case 1
      iAssertCase = 1
    case 2
  endsw
  if p4 == 1 && iAssertCase == 0 then
    prints "assert error, case 1 did not execute with trailing empty case\n"
    exitnow(-1)
  endif
  if p4 != 1 && iAssertCase != 0 then
    prints "assert error, trailing empty case executed unexpectedly\n"
    exitnow(-1)
  endif
  prints "test10 passed\n"
endin

// switch in UDO preserves valid xout type and branch behavior
instr 11
  i1 switch_xout_value 1
  i2 switch_xout_value 2
  i3 switch_xout_value 3
  i4 switch_xout_value 4
  if i1 != 11 || i2 != 22 || i3 != 33 || i4 != 44 then
    prints "assert error, switch_xout_value produced wrong results\n"
    exitnow(-1)
  endif
  prints "test11 passed\n"
endin

// empty cases in UDO do not crash and route correctly to default
instr 12
  i1 switch_empty_case_udo 1
  i2 switch_empty_case_udo 2
  i3 switch_empty_case_udo 3
  if i1 != 0 || i2 != 0 || i3 != 1 then
    prints "assert error, switch_empty_case_udo produced wrong results\n"
    exitnow(-1)
  endif
  prints "test12 passed\n"
endin

// switch inside for-loop executes all iterations and reaches code after loop
instr 13
  iLoopCount = 0
  iBranchSum = 0
  iAfterLoop = 0
  for iCond in [1,2,3,4] do
    switch iCond
      case 1
        iBranchSum += 1
      case 2
        iBranchSum += 2
      case 3
        iBranchSum += 3
      default
        iBranchSum += 4
    endsw
    iLoopCount += 1
  od
  iAfterLoop = 1
  if iLoopCount != 4 then
    prints "assert error, loop exited early with switch\n"
    exitnow(-1)
  endif
  if iBranchSum != 10 then
    prints "assert error, switch branches in loop were incorrect\n"
    exitnow(-1)
  endif
  if iAfterLoop == 0 then
    prints "assert error, code after loop was not reached\n"
    exitnow(-1)
  endif
  prints "test13 passed\n"
endin

// switch with only default case executes default body and continues
instr 14
  iDefault = 0
  iAfterSwitch = 0
  switch 123
    default
      iDefault = 1
  endsw
  iAfterSwitch = 1
  if iDefault == 0 then
    prints "assert error, default-only switch did not execute default\n"
    exitnow(-1)
  endif
  if iAfterSwitch == 0 then
    prints "assert error, default-only switch did not continue control flow\n"
    exitnow(-1)
  endif
  prints "test14 passed\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0 1
i 2 0 0
i 3 0 0
i 4 0 0
i 5 0 0.1
i 6 0 0 1
i 6 0 0 2
i 6 0 0 3
i 7 0 0 1
i 7 0 0 2
i 7 0 0 3
i 8 0 0 1
i 8 0 0 2
i 9 0 0 1
i 9 0 0 2
i 10 0 0 1
i 10 0 0 2
i 10 0 0 3
i 11 0 0
i 12 0 0
i 13 0 0
i 14 0 0
</CsScore>
</CsoundSynthesizer>
