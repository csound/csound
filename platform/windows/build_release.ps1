Write-Host "Configure build"

$InstallPrefix = Join-Path (Get-Location) bin\windows\release

cmake -B build -S . -DUSE_VCPKG=1 -DCUSTOM_CMAKE=".\platform\windows\Custom-vs.cmake" -DCMAKE_INSTALL_PREFIX:PATH=$InstallPrefix

Write-Host "Build Csound"

cmake --build build --config Release

Write-Host "Install Csound"

cmake --build build --target install --config Release
