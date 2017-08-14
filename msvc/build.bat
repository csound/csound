@echo off
echo "usage: build [cmake_generator cmake_toolset]"
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 %1 %2
powershell -ExecutionPolicy ByPass -File generateProject.ps1 %1 %2
cmake --build csound-vs --config Release
