@echo off
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 16 2019"
cmake -DBUILD_PYTHON_OPCODES=1 --build csound-vs --config Release
