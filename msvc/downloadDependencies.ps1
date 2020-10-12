param
(
    [string]$vsGenerator = "Visual Studio 16 2019"
)

# Using a custom triplet due to mixed static and dynamic dependencies
# Only libsndfile is static, all others are dynamically linked
$targetTriplet = "x64-windows-csound"

echo "Downloading Csound dependencies..."
echo "vsGenerator: $vsGenerator"
echo "Build type: $targetTriplet"

$webclient = New-Object System.Net.WebClient
$startTime = (Get-Date).TimeOfDay
$currentDir = Split-Path $MyInvocation.MyCommand.Path
$cacheDir = $currentDir + "\cache\"
$vcpkgDir = ""

# Find VCPKG from path if it already exists
# Otherwise use the local Csound version that will be installed
$systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source
$vcpkgDir = ""

# Test if VCPKG is already installed on system
# Download locally to outside the repo
if ($systemVCPKG) {
    echo "vcpkg already installed on system, updating"
    $vcpkgDir = Split-Path -Parent $systemVCPKG
    cd $vcpkgDir
    # Update and rebuild vcpkg
    git pull
    bootstrap-vcpkg.bat
    # Remove any outdated packages (they will be installed again below)
    vcpkg remove --outdated --recurse
    vcpkg update # Not really functional it seems yet
    cd $currentDir
}
elseif (Test-Path "..\..\vcpkg") {
    cd ..\..\vcpkg
    $env:Path += ";" + $(Get-Location)
    $vcpkgDir = $(Get-Location)
    [Environment]::SetEnvironmentVariable("VCPKGDir", $env:vcpkgDir, [EnvironmentVariableTarget]::User)
    echo "vcpkg already installed locally, updating"
    # Update and rebuild vcpkg
    git pull
    bootstrap-vcpkg.bat
    # Remove any outdated packages (they will be installed again below)
    vcpkg remove --outdated --recurse
    vcpkg update
    cd $currentDir
}
else {
    cd ..\..
    echo "vcpkg missing, downloading and installing..."
    git clone --depth 1 http://github.com/Microsoft/vcpkg.git
    cd vcpkg
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
    eigen3 fltk zlib libflac libogg libvorbis libsndfile libsamplerate portmidi portaudio liblo hdf5 dirent libstk fluidsynth `
    --overlay-triplets=.

echo "Downloading and installing non-VCPKG packages..."

choco install swig -y --version=4.0.1 --allow-downgrade
choco upgrade swig -y --version=4.0.1 --allow-downgrade

choco install winflexbison -y
choco upgrade winflexbison -y

cd $currentDir

$depsDir = $currentDir + "\deps\"
$depsBinDir = $depsDir + "bin\"
$depsLibDir = $depsDir + "lib\"
$depsIncDir = $depsDir + "include\"

mkdir cache -ErrorAction SilentlyContinue
mkdir deps -ErrorAction SilentlyContinue
mkdir $depsLibDir -ErrorAction SilentlyContinue
mkdir $depsBinDir -ErrorAction SilentlyContinue
mkdir $depsIncDir -ErrorAction SilentlyContinue

# Download ASIO SDK and extract
$asioURI = "http://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"
$fileName = Split-Path -Leaf $asioURI
$cachedFile = $cacheDir + $fileName
if (Test-Path $cachedFile -PathType Leaf) {
    echo "Already downloaded file: $fileName"
}
else {
    echo "Downloading: " $asioURI
    $webclient.DownloadFile($asioURI, $cachedFile)
}

# Extract file
if ($PSVersionTable.PSVersion.Major -gt 3) {
    Expand-Archive $cachedFile -DestinationPath $depsDir -Force
}
else {
    New-Item $destDir -ItemType directory -Force
    Expand-Archive $cachedFile -OutputPath $depsDir -Force
}

cd $depsDir

echo "PortAudio..."
if (Test-Path "portaudio") {
    cd portaudio
    git pull
    cd ..
    echo "Portaudio already downloaded, updated."
}
else {
    git clone --depth=1 "https://github.com/PortAudio/portaudio.git"
}

copy portaudio\include\portaudio.h -Destination $depsIncDir -Force
rm -Path portaudioBuild -Force -Recurse -ErrorAction SilentlyContinue
mkdir portaudioBuild -ErrorAction SilentlyContinue
cd portaudioBuild
cmake ..\portaudio -G $vsGenerator -DCMAKE_BUILD_TYPE="Release" -DPA_USE_ASIO=1
cmake --build . --config Release
copy .\Release\portaudio_x64.dll -Destination $depsBinDir -Force
copy .\Release\portaudio_x64.lib -Destination $depsLibDir -Force

# Add deps bin directory to the system path if not already there
if ($env:Path.Contains($depsBinDir)) {
    echo "Already added dependency bin dir to path"
}
else {
    # For this session add to path var
    $env:Path += ";" + $depsBinDir

    # Permanently add to system path
    [Environment]::SetEnvironmentVariable("Path", $env:Path,
        [EnvironmentVariableTarget]::User)

    echo "Added dependency bin dir to path: $depsBinDir"
}

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

# Default to Release build type. Note: ReleaseWithDebInfo is broken as VCPKG does not currently support this mode properly
cmake ..\.. -DBUILD_PYTHON_OPCODES=1 -G $vsGenerator `
    -Wno-dev -Wdeprecated `
    -DCMAKE_BUILD_TYPE="Release" `
    -DVCPKG_TARGET_TRIPLET="$targetTriplet" `
    -DCMAKE_TOOLCHAIN_FILE="$vcpkgCmake" `
    -DCMAKE_INSTALL_PREFIX=dist `
    -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
