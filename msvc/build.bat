@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 16 2019"
powershell -ExecutionPolicy ByPass -File generateProject.ps1 -vsGenerator "Visual Studio 16 2019"
cmake --build csound-vs --config RelWithDebInfo
