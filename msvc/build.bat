@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 16 2019"
cmake --build csound-vs --config Release -- /m:6
