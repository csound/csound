#!/usr/bin/lua

require "luaCsnd"

csound = luaCsnd.Csound()
args = luaCsnd.CsoundArgVList()
args_ = {"csound", "-o", "test.wav", "examples/trapped.csd"}
for i,v in ipairs(args_) do
    args:Append(v)
end
csound:Compile(args.argc(args), args.argv(args))
pt = luaCsnd.CsoundPerformanceThread(csound)
pt:SetScoreOffsetSeconds(21.5)
pt:Play()
luaCsnd.csoundSleep(10000)
pt:SetScoreOffsetSeconds(152)
luaCsnd.csoundSleep(10000)
pt:Stop()
pt:Join()
csound:Reset()
args:Clear()

