param
(
    [string]$vsGenerator="Visual Studio 15 2017 Win64",
    [string]$vsToolset="v141"
)
echo "Generating Csound Visual Studio solution..."

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
 -Wno-dev -Wdeprecated `
 -DSTK_LOCAL:BOOL="ON" `
 -DCMAKE_BUILD_TYPE="RelWithDebInfo" `
 -DVCPKG_TARGET_TRIPLET=x64-windows-static `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 -DCMAKE_REQUIRED_INCLUDES="..\deps\include" `
