@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1
powershell -ExecutionPolicy ByPass -File generateProject.ps1
cmake --build csound-vs --config Release
