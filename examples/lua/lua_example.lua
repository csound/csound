#!/usr/bin/lua

require "luaCsnd6"

csound = luaCsnd6.Csound()
args = luaCsnd6.CsoundArgVList()
args_ = {"csound", "-o", "test.wav", "../trapped.csd"}
for i,v in ipairs(args_) do
    args:Append(v)
end
csound:Compile(args.argc(args), args.argv(args))
csound:startPerformance(nil)
--csound:startPerformance()
pt = luaCsnd6.CsoundPerformanceThread(csound)
pt:SetScoreOffsetSeconds(21.5)
pt:Play()
luaCsnd6.csoundSleep(10000)
pt:SetScoreOffsetSeconds(152)
luaCsnd6.csoundSleep(10000)
pt:Stop()
pt:Join()
csound:Reset()
args:Clear()

