<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1
ksmps = 32

instr FailNow
    exitnow(-1)
endin

opcode inc_k, k, K
    kval xin
    kout = kval + 1
    xout kout
endop

opcode sum_diff_K, kk, KK
    kin1, kin2 xin
    ksum = kin1 + kin2
    kdiff = kin1 - kin2
    xout ksum, kdiff
endop

gkResult init -1

instr 1
    gkInput init 5
    kresult = inc_k(gkInput)

    if (kresult != 6) then
        printks("ERROR: Return value failed for inc_k. kresult=%g\n", .001, kresult)
        schedulek("FailNow", 0, 0)
        turnoff
    endif

    gkResult = kresult
endin

instr 2
    k1 init 10
    k2 init 3
    ksum, kdiff = sum_diff_K(k1, k2)

    if (ksum != 13 || kdiff != 7) then
        printks("ERROR: SumDiffK outputs wrong values. ksum=%g kdiff=%g\n", .001, ksum, kdiff)
        schedulek("FailNow", 0, 0)
        turnoff
    endif
endin

instr 3
    if (gkResult != 6) then
        printks("ERROR: Global K result incorrect. gkResult=%g\n", .001, gkResult)
        schedulek("FailNow", 0, 0)
        turnoff
    endif
endin

opcode TestKIO,K,K
 kval xin
 kval = 0
  xout kval
endop

instr 4
input:i = 1
output:k = TestKIO(input)
if input:i != i(output:k) then
  exitnow(-1)
endif
endin

</CsInstruments>
<CsScore>
i1 0 0.01
i2 0 0.01
i3 0 0.01
i4 0 0.01
</CsScore>
</CsoundSynthesizer>
