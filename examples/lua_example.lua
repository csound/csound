#!/usr/bin/lua

csnd_init = loadlib("./lib_csnd.so", "Csnd_Init")
csnd_init()

csound = csnd.Csound()
args = csnd.CsoundArgVList()
args_ = {"csound", "-o", "dac", "examples/trapped.csd"}
for i in args_ do
    args.Append(args, args_[i])
end
csound.Compile(csound, args.argc(args), args.argv(args))
pt = csnd.CsoundPerformanceThread(csound)
pt.SetScoreOffsetSeconds(pt, 21.5)
pt.Play(pt)
csnd.csoundSleep(10000)
pt.SetScoreOffsetSeconds(pt, 152)
csnd.csoundSleep(10000)
pt.Stop(pt)
pt.Join(pt)
csound.Reset(csound)
args.Clear(args)

