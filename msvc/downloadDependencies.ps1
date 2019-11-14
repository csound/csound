param
(
    [string]$vsGenerator="Visual Studio 16 2019",
    [switch]$buildStatic=$false,
    [switch]$staticCRT=$false
)

# The default build is to generate dynamic libs that use the CRT dynamically 
if ($buildStatic) {
    $targetTriplet = "x64-windows-static"

    if ($staticCRT == $false) {
        echo "Cannot have dynamic CRT linkage with static Csound build"
        $staticCRT = $true
    }
} 
else {
    if ($staticCRT) {
        $targetTriplet = "x64-windows-crt-mt"
    }
    else {
        $targetTriplet = "x64-windows"
    }
}

echo "Downloading Csound dependencies..."
echo "vsGenerator: $vsGenerator"
echo "Build type: $targetTriplet"

$startTime = (Get-Date).TimeOfDay
# $webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path
$cacheDir = $currentDir + "\cache\"
$csoundDir = $currentDir + "\.."
$vcpkgDir = ""

# Find VCPKG from path if it already exists
# Otherwise use the local Csound version that will be installed
$systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source
$vcpkgDir = ""

# Test if VCPKG is already installed on system
# Download locally to outside the repo
if ($systemVCPKG)
{
    echo "vcpkg already installed on system, updating"
    $vcpkgDir = Split-Path -Parent $systemVCPKG
    cd $vcpkgDir
    # Update and rebuild vcpkg
    git pull
    git checkout 7b7908b
    bootstrap-vcpkg.bat
    # Remove any outdated packages (they will be installed again below)
    vcpkg remove --outdated --recurse
    vcpkg update # Not really functional it seems yet
    cd $currentDir
}
elseif (Test-Path "..\..\vcpkg")
{
    cd ..\..\vcpkg
    $env:Path += ";" + $(Get-Location)
    $vcpkgDir = $(Get-Location)
    [Environment]::SetEnvironmentVariable("VCPKGDir", $env:vcpkgDir, [EnvironmentVariableTarget]::User)
    echo "vcpkg already installed locally, updating"
    # Update and rebuild vcpkg
    git pull
    git checkout 7b7908b
    bootstrap-vcpkg.bat
    # Remove any outdated packages (they will be installed again below)
    vcpkg remove --outdated --recurse
    vcpkg update
    cd $currentDir
}
else
{
    cd ..\..
    echo "vcpkg missing, downloading and installing..."
    git clone --depth 1 http://github.com/Microsoft/vcpkg.git
    cd vcpkg
    git checkout 7b7908b
    $env:Path += ";" + $(Get-Location)
    $vcpkgDir = $(Get-Location)
    [Environment]::SetEnvironmentVariable("VCPKGDir", $env:vcpkgDir, [EnvironmentVariableTarget]::User)
    bootstrap-vcpkg.bat
    vcpkg integrate install
    cd $currentDir
}

# Generate VCPKG AlwaysAllowDownloads file if needed
New-Item -type file $vcpkgDir\downloads\AlwaysAllowDownloads -errorAction SilentlyContinue | Out-Null

# Download all vcpkg packages available
echo "Downloading VC packages..."

# Download asiosdk and extract before doing portaudio installation
if (-not (Test-Path $vcpkgDir/buildtrees/portaudio/src/asiosdk)) {
    echo "ASIOSDK not installed into VCPKG"
    if (-not (Test-Path .\cache\asiosdk.zip)) {
        Invoke-WebRequest https://www.steinberg.net/asiosdk -OutFile cache/asiosdk.zip
    }
    Expand-Archive -Path cache/asiosdk.zip -DestinationPath $vcpkgDir/buildtrees/portaudio/src -Force
    Move-Item -Path $vcpkgDir/buildtrees/portaudio/src/asiosdk_* -Destination $vcpkgDir/buildtrees/portaudio/src/asiosdk -ErrorAction SilentlyContinue
    # Remove portaudio and it will get rebuilt with asio support in the next step
    vcpkg --triplet $targetTriplet remove portaudio --overlay-triplets=.
}

vcpkg --triplet $targetTriplet install `
    eigen3 fltk zlib libflac libogg libvorbis libsndfile libsamplerate portmidi portaudio liblo hdf5 dirent libstk fluidsynth `
    --overlay-triplets=.

echo "Downloading and installing non-VCPKG packages..."

choco install swig -y
choco upgrade swig -y

choco install winflexbison -y
choco upgrade winflexbison -y

$endTime = (Get-Date).TimeOfDay
$duration = $endTime - $startTime

echo "Total duration: $($duration.TotalMinutes) minutes"
echo "-------------------------------------------------"
echo "Generating Csound Visual Studio solution..."
echo "vsGenerator: $vsGenerator"

$vcpkgCmake = "$vcpkgDir\scripts\buildsystems\vcpkg.cmake"
echo "VCPKG script: '$vcpkgCmake'"

cd $currentDir
mkdir csound-vs -ErrorAction SilentlyContinue
cd csound-vs -ErrorAction SilentlyContinue

$buildSharedLibs = $(if ($buildStatic) { "OFF" } else { "ON" })
$useStaticCRT = $(if ($staticCRT) { "ON" } else { "OFF" })

# Default to Release build type. Note: ReleaseWithDebInfo is broken as VCPKG does not currently support this mode properly
cmake ..\.. -G $vsGenerator `
 -Wno-dev -Wdeprecated `
 -DCMAKE_BUILD_TYPE="Release" `
 -DVCPKG_TARGET_TRIPLET="$targetTriplet" `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 -DBUILD_SHARED_LIBS=$buildSharedLibs `
 -DSTATIC_CRT=$useStaticCRT
