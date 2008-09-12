#!/usr/bin/lua

-- On Linux this should work:
-- require "csnd"

require "csnd"

csound = csnd.Csound()
args = csnd.CsoundArgVList()
args_ = {"csound", "-o", "test.wav", "examples/trapped.csd"}
for i,v in ipairs(args_) do
    args:Append(v)
end
csound:Compile(args.argc(args), args.argv(args))
pt = csnd.CsoundPerformanceThread(csound)
pt:SetScoreOffsetSeconds(21.5)
pt:Play()
csnd.csoundSleep(10000)
pt:SetScoreOffsetSeconds(152)
csnd.csoundSleep(10000)
pt:Stop()
pt:Join()
csound:Reset()
args:Clear()

