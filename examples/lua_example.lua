#!/usr/bin/lua

-- On Linux this should work:
-- require "csnd"

-- On Windows this is needed because the _csnd.dll name
-- does not match the csnd module:
csnd_init = package.loadlib("_csnd.dll", "Csnd_Init")
csnd_init()

csound = csnd.Csound()
args = csnd.CsoundArgVList()
args_ = {"csound", "-o", "dac", "examples/trapped.csd"}
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

