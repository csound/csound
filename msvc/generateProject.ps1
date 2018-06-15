param
(
    [string]$vsGenerator="Visual Studio 15 2017 Win64",
    [string]$vsToolset="v141",
    [string]$vstSdkHome
)
echo "Generating Csound Visual Studio solution..."

echo "vsGenerator: $vsGenerator"
echo "vsToolset:   $vsToolset"
echo "vstSdkHome:  $vstSdkHome"

$vcpkgCmake = ""

# Read in VCPKG directory from env variable
if ($systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source)
{
    echo "vcpkg already installed on system, finding directory"
    $vcpkgDir = Split-Path -Parent $systemVCPKG
    echo "vckpgDir: $vcpkgDir"
    $vcpkgCmake = "$vcpkgDir\scripts\buildsystems\vcpkg.cmake"
}
elseif (Test-Path "..\..\vcpkg")
{
	echo "using local VCPKG cmake file"
	$vcpkgCmake = "..\..\vcpkg\scripts\buildsystems\vcpkg.cmake"
	$vcpkgCmake = [System.IO.Path]::GetFullPath($vcpkgCmake)
    $env:PATH = "$vcpkgDir" + "\buildtrees\fltk\x64-windows-rel\bin;" + "$vcpkgDir" + "\buildtrees\fltk\x64-windows-rel\fluid;" + "$vcpkgDir" + "\installed\x64-windows\bin;" + $env:PATH
    echo $env:PATH
}
else
{
    # VCPKG not installed globally or locally, abort
    echo "Please run the 'downloadDependencies.bat' script first!"
    exit
}

echo "VCPKG script: '$vcpkgCmake'"

mkdir csound-vs -ErrorAction SilentlyContinue
cd csound-vs -ErrorAction SilentlyContinue
dir ..\deps
$linkPath = Resolve-Path -Path "..\deps\link"
cmake ..\.. -G $vsGenerator -T $vsToolset  `
 -Wno-dev -Wdeprecated `
 -DABLETON_LINK_HOME="$linkPath" `
 -DBUILD_ABLETON_LINK_OPCODES:BOOL=On `
 -DSTK_LOCAL:BOOL="ON" `
 -DCMAKE_BUILD_TYPE="RelWithDebInfo" `
 -DVCPKG_TARGET_TRIPLET=x64-windows-static `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 -DCMAKE_REQUIRED_INCLUDES="..\deps\include" `
 -DEIGEN3_INCLUDE_PATH:PATH="$vcpkgDir\packages\eigen3_x64-windows-static\include" `
 -DVSTSDK2X_INCLUDE_DIR:PATH="$vstSdkHome" `
 -DBUILD_CSOUND_VST:BOOL=ON `
 -DBUILD_PADSYNTH_OPCODES:BOOL=ON `
 -DBUILD_LINEAR_ALGEBRA_OPCODES:BOOL=ON `
 -DBUILD_VST4CS_OPCODES:BOOL=ON
