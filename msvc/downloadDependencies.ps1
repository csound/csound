param
(
    [string]$vsGenerator="Visual Studio 16 2019"
)

# Using a custom triplet due to mixed static and dynamic dependencies
# Only libsndfile is static, all others are dynamically linked
$targetTriplet = "x64-windows-csound"

echo "Downloading Csound dependencies..."
echo "vsGenerator: $vsGenerator"
echo "Build type: $targetTriplet"

$startTime = (Get-Date).TimeOfDay
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
    New-Item -ItemType Directory -Force -Path $cacheDir
    if (-not (Test-Path .\cache\asiosdk.zip)) {
        Invoke-WebRequest https://www.steinberg.net/asiosdk -OutFile cache/asiosdk.zip
    }
    Expand-Archive -Path cache/asiosdk.zip -DestinationPath $vcpkgDir/buildtrees/portaudio/src -Force
    Move-Item -Path $vcpkgDir/buildtrees/portaudio/src/asiosdk_* -Destination $vcpkgDir/buildtrees/portaudio/src/asiosdk -ErrorAction SilentlyContinue
    # Remove portaudio and it will get rebuilt with asio support in the next step
    vcpkg --triplet $targetTriplet remove portaudio --overlay-triplets=.
}

vcpkg --triplet $targetTriplet install `
    eigen3 fltk zlib libflac libogg libvorbis libsndfile libsamplerate portaudio liblo hdf5 dirent libstk fluidsynth `
    --overlay-triplets=.

echo "Downloading and installing non-VCPKG packages..."

choco install swig -y --version=4.0.1 --allow-downgrade
choco upgrade swig -y --version=4.0.1 --allow-downgrade

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

# TEMPORARILY USE THE FOLLOWING SELF-BUILT PORTMIDI UNTIL VCPKG PROVIDES ONE
# THAT IS UP TO DATE

$depsDir = $currentDir + "\deps\"
$stageDir = $currentDir + "\staging\"
$depsBinDir = $depsDir + "bin\"
$depsLibDir = $depsDir + "lib\"
$depsIncDir = $depsDir + "include\"

mkdir cache -ErrorAction SilentlyContinue
mkdir deps -ErrorAction SilentlyContinue
mkdir $depsLibDir -ErrorAction SilentlyContinue
mkdir $depsBinDir -ErrorAction SilentlyContinue
mkdir $depsIncDir -ErrorAction SilentlyContinue
mkdir staging -ErrorAction SilentlyContinue

cd $depsDir

if (Test-Path "portmidi")
{
  cd portmidi
  svn update  
  cd ..
  echo "Portmidi already downloaded, updated"
}
else
{
  svn checkout "https://svn.code.sf.net/p/portmedia/code" portmidi
}

cd portmidi\portmidi\trunk
rm -Path build -Force -Recurse -ErrorAction SilentlyContinue
mkdir build -ErrorAction SilentlyContinue
cd build
cmake .. -G $vsGenerator -DCMAKE_BUILD_TYPE="Release"
cmake --build . --config Release
copy .\Release\portmidi.dll -Destination $depsBinDir -Force
copy .\Release\portmidi.lib -Destination $depsLibDir -Force
copy .\Release\portmidi_s.lib -Destination $depsLibDir -Force
copy .\Release\pmjni.dll -Destination $depsBinDir -Force
copy .\Release\pmjni.lib -Destination $depsLibDir -Force
copy ..\pm_common\portmidi.h -Destination $depsIncDir -Force
copy ..\porttime\porttime.h -Destination $depsIncDir -Force

# END CUSTOM PORTMIDI #


cd $currentDir
mkdir csound-vs -ErrorAction SilentlyContinue
cd csound-vs -ErrorAction SilentlyContinue

# Default to Release build type. Note: ReleaseWithDebInfo is broken as VCPKG does not currently support this mode properly
cmake ..\.. -G $vsGenerator `
 -Wno-dev -Wdeprecated `
 -DCMAKE_BUILD_TYPE="Release" `
 -DVCPKG_TARGET_TRIPLET="$targetTriplet" `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 # -DSTATIC_CRT="ON"
