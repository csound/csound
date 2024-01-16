Write-Host "Install csound dependencies"

choco install -y winflexbison3 innosetup llvm javaruntime jdk8 swig python
choco install -y cmake --installargs '"ADD_CMAKE_TO_PATH=System"'
choco install -y visualstudio2022community --package-parameters "add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --includeOptional --passive --locale en-US"
choco install -y visualstudio2022buildtools

Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
refreshenv

Write-Host "Bootstrap VCPKG"

..\..\vcpkg\bootstrap-vcpkg.bat

Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
refreshenv
