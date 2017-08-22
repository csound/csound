@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 %1 %2
