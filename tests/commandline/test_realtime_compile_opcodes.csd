<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null -d --realtime
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

giStringResult init 0
giFileResult init 0

instr 1
  iStringStatus compilestr {{
giStringResult = 1

instr RealtimeCompiledString
endin
}}

  if (iStringStatus != 0) then
    prints "compilestr failed: status=%d\n", iStringStatus
    exitnow(1)
  endif

  Scwd pwd
  Sfile sprintf "%s/test_realtime_compile_opcodes.orc", Scwd
  if (filevalid(Sfile) == 0) then
    Sfile sprintf "%s/tests/commandline/test_realtime_compile_opcodes.orc", Scwd
  endif
  iFileStatus compileorc Sfile

  if (iFileStatus != 0) then
    prints "compileorc failed: status=%d\n", iFileStatus
    exitnow(1)
  endif

  iStringInstr nstrnum "RealtimeCompiledString"
  iFileInstr nstrnum "RealtimeCompiledFile"

  if (iStringInstr <= 0 || iFileInstr <= 0) then
    prints "compiled instruments unavailable: string=%d file=%d\n", \
      iStringInstr, iFileInstr
    exitnow(1)
  endif

  if (giStringResult != 1 || giFileResult != 1) then
    prints "compiled global init did not run: string=%d file=%d\n", \
      giStringResult, giFileResult
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
