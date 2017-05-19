echo "Generating Csound VS project..."
$vcpkgCmake = ""

# Read in VCPKG directory from env variable
if ($systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source)
{
    echo "vcpkg already installed on system, finding directory"
    $vcpkgDir = Split-Path -Parent $systemVCPKG
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
cd csound-vs -InformationAction SilentlyContinue

cmake ..\.. -G "Visual Studio 15 2017 Win64" `
 -Wdev -Wdeprecated `
 -DCMAKE_BUILD_TYPE="Release" `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake"

