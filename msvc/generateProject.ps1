param
(
    [string]$vsGenerator="Visual Studio 15 2017 Win64",
    [string]$vsToolset="v141"
)
echo "Generating Csound VS project..."

echo "vsGenerator: $vsGenerator"
echo "vsToolset:   $vsToolset"

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

cmake ..\.. -G $vsGenerator -T $vsToolset  `
 -Wdev -Wdeprecated `
 -DSTK_LOCAL:BOOL="ON" `
 -DCMAKE_BUILD_TYPE="Release" `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 -DCMAKE_REQUIRED_INCLUDES="..\deps\include" `
 -DEIGEN3_INCLUDE_PATH:PATH=$vcpkgDir\packages\eigen3_x64-windows\include
