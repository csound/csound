@echo off
rem powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1
powershell -ExecutionPolicy ByPass -File generateProject-mkg.ps1
cmake --build csound-vs --config RelWithDebInfo
