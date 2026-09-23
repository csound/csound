<CsTest>
description = "directory preserves sorted results, array copies and repeated initialization"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
giChecks init 0

instr 1
  fprints "directory_result_state/directory_z.dirtest", "z"
  fprints "directory_result_state/directory_a.dirtest", "a"
  fprints "directory_result_state/directory_m_long_name.dirtest", "m"
  ficlose "directory_result_state/directory_z.dirtest"
  ficlose "directory_result_state/directory_a.dirtest"
  ficlose "directory_result_state/directory_m_long_name.dirtest"
endin

instr 2
  iPrefix = strlen("directory_result_state") + 1
  SNames[] fillarray "old", "old value with a longer buffer", "old"
  SSaved[] init 0
  iStep = 0
again:
  iPhase = iStep % 3
  SFilter strcpy ".dirtest"
  if iPhase == 1 then
    SFilter strcpy ".directory_no_matches"
  endif
  SNames directory "directory_result_state", SFilter
  if iPhase == 1 then
    if lenarray(SNames) != 0 || lenarray(SSaved) != 3 then
      prints "directory empty results or saved copy have the wrong size\n"
      exitnow -1
    endif
    if strindex(SSaved[0], "directory_a.dirtest") != iPrefix then
      prints "directory changed a saved array copy\n"
      exitnow -1
    endif
  else
    if lenarray(SNames) != 3 then
      prints "directory returned the wrong number of matching files\n"
      exitnow -1
    endif
    if strindex(SNames[0], "directory_a.dirtest") != iPrefix || strindex(SNames[1], "directory_m_long_name.dirtest") != iPrefix || strindex(SNames[2], "directory_z.dirtest") != iPrefix then
      prints "directory returned the wrong names or sort order\n"
      exitnow -1
    endif
    SSaved = SNames
  endif
  iStep += 1
  if iStep < 30 igoto again
  giChecks += 1
endin

instr 3
  iPrefix = strlen("directory_result_state") + 1
  kCycle timeinstk
  if kCycle == 3 then
    reinit read
  endif
read:
  SNames[] directory "directory_result_state", ".dirtest"
  if lenarray(SNames) != 3 || strindex(SNames[0], "directory_a.dirtest") != iPrefix then
    exitnow -1
  endif
  giChecks += 1
  rireturn
endin

instr 99
  if giChecks != 4 then
    prints "directory checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .001
i2 .02 .001
i3 .03 .006
i99 .05 .001
</CsScore>
</CsoundSynthesizer>
