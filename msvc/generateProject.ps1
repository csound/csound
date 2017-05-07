echo "Generating Csound VS project..."
$vcpkgCmake = ""

# Read in VCPKG directory from env variable
if ($systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source)
{
    echo "vcpkg already installed on system, finding directory"
    $vcpkgDir = Split-Path -Parent $systemVCPKG
    $vcpkgCmake = "$vcpkgDir\scripts\buildsystems\vcpkg.cmake"
}
else 
{
    echo "checking VCPKG dir in environmental variable"
    #$vcpkgDir = $env:VCPKGDir
    $vcpkgCmake = "..\vcpkg\scripts\buildsystems\vcpkg.cmake"
}

echo "VCPKG script: '$vcpkgCmake'"

mkdir csound-vs -ErrorAction SilentlyContinue
cd csound-vs -InformationAction SilentlyContinue

cmake ..\.. -G "Visual Studio 15 2017 Win64" `
 -Wdev -Wdeprecated `
 -DCMAKE_BUILD_TYPE="Release" `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 -DHAVE_BIG_ENDIAN=0 `
 -DCMAKE_16BIT_TYPE="unsigned short" `
 -DUSE_ALSA=0 `
 -DUSE_AUDIOUNIT=0 `
 -DUSE_COREMIDI=0 `
 -DUSE_CURL=0 `
 -DUSE_DOUBLE=1 `
 -DUSE_GETTEXT=0 `
 -DUSE_JACK=0 `
 -DUSE_PULSEAUDIO=0 `
 -DBUILD_INSTALLER=1 `
 -DBUILD_FLUID_OPCODES=0 `
 -DBUILD_LUA_OPCODES=0 `
 -DBUILD_LUA_INTERFACE=0 `
 -DBUILD_CSOUND_AC_LUA_INTERFACE=0 `
 -DBUILD_PD_CLASS=0 `
 -DLIBSNDFILE_LIBRARY="..\deps\lib\libsndfile-1.lib" `
 -DSWIG_DIR="C:\msys64\mingw64\share\swig\3.0.6" `
 -DFLEX_EXECUTABLE="..\deps\win_flex_bison\win_flex.exe" `
 -DBISON_EXECUTABLE="..\deps\win_flex_bison\win_bison.exe" `
 -DPORTAUDIO_INCLUDE_PATH="..\staging\portaudio\include" `
 -DPORTAUDIO_LIBRARY="..\staging\portaudioBuild\Release\portaudio_x64.lib"
