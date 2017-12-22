@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140_xp"
powershell -ExecutionPolicy ByPass -File generateProject.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140_xp"
cmake --build csound-vs --config RelWithDebInfo
