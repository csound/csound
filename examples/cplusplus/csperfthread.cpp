#include "csound.hpp"
#include "csPerfThread.hpp"
#include "csPerfThread.cpp"

int useThreads = true;

int main(int argc, char *argv[])
{
    Csound csound;
    csound.Compile(argc,argv);
    csound.Start();
    if (useThreads) {
        CsoundPerformanceThread performanceThread(csound.GetCsound());
        performanceThread.Play(); 
        performanceThread.Join();  
    } else {
        while (csound.PerformKsmps() == 0) {}
    }
 }
