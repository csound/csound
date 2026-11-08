<CsTest>
description = "ftexists quietly queries live, missing, freed, and deferred tables"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --defer-gen1
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
giLive ftgen 100, 0, 8, -2, 1
giLoaded ftgen 101, 0, 8, -2, 0
giFreed ftgen 102, 0, 8, -2, 0
; This query must not try to open the deferred sound file.
giDeferred ftgen 103, 0, 0, 1, "ftexists_unopened_deferred.wav", 0, 0, 0
ftsave "test_ftexists_queries.ftsave", 0, giLive
ftload "test_ftexists_queries.ftsave", 0, giLoaded
ftfree giFreed, 0
gkChecks init 0

instr 1
  iQueries[] fillarray giLive, giLive+.5, giLoaded, giDeferred, giFreed, 99, 987654, 0, -1, -2, 1e20
  iExpected[] fillarray 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0
  iIndex = 0
  while iIndex < lenarray(iQueries) do
    iExists ftexists iQueries[iIndex]
    if iExists != iExpected[iIndex] then
      prints "incorrect init-time table existence result\n"
      exitnow -1
    endif
    iIndex += 1
  od

  kCycle timeinstk
  kIndex = kCycle-1
  kQuery = iQueries[kIndex]
  kExists ftexists kQuery
  if kExists != iExpected[kIndex] then
    printks "incorrect control-time table existence result\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  if kCycle == lenarray(iQueries) then
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 11 then
    prints "table existence checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .25
i99 .3 .01
</CsScore>
</CsoundSynthesizer>
