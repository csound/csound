#!/usr/bin/lua

require "luaCsnd6"

csound = luaCsnd6.Csound()
args = luaCsnd6.CsoundArgVList()
args_ = {"csound", "-dfo", "test.wav", "examples/trapped.csd"}
for i,v in ipairs(args_) do
    args:Append(v)
end
csound:Compile(args.argc(args), args.argv(args))
csound:Start()
--while csound:PerformKsmps() == 0 do
--end
----[[
pt = luaCsnd6.CsoundPerformanceThread(csound)
-- pt:SetScoreOffsetSeconds(21.5)
pt:Play()
--uaCsnd6.csoundSleep(10000)
--pt:SetScoreOffsetSeconds(152)
--luaCsnd6.csoundSleep(10000)
--pt:Stop()
pt:Join()
--]]
csound:Cleanup()
csound:Reset()
args:Clear()

