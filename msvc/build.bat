@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 15 2017 Win64" -vsToolset "v141"
powershell -ExecutionPolicy ByPass -File generateProject.ps1 -vsGenerator "Visual Studio 15 2017 Win64" -vsToolset "v141"
cmake --build csound-vs --config RelWithDebInfo
