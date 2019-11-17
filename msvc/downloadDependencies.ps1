param
(
    [string]$vsGenerator="Visual Studio 16 2019",
    [string]$vsToolset="v142"
)

echo "Downloading Csound dependencies..."
echo "vsGenerator: $vsGenerator"
echo "vsToolset:   $vsToolset"

$startTime = (Get-Date).TimeOfDay

$webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path
$cacheDir = $currentDir + "\cache\"
$depsDir = $currentDir + "\deps\"
$stageDir = $currentDir + "\staging\"
$depsBinDir = $depsDir + "bin\"
$depsLibDir = $depsDir + "lib\"
$depsIncDir = $depsDir + "include\"
$csoundDir = $currentDir + "\.."
$vcpkgDir = ""

# Metrics
$vcpkgTiming = 0
$buildTiming = 0
$cmakeTiming = 0

# Add to path to call tools
$env:Path += ";"+ $depsDir + ";"

# Find VCPKG from path if it already exists
# Otherwise use the local Csound version that will be installed
$systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source

# Test if VCPKG is already installed on system
# Download locally to outside the repo
if ($systemVCPKG)
{
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
elseif (Test-Path "..\..\vcpkg")
{
    cd ..\..\vcpkg
    $env:Path += ";" + $(Get-Location)
    $vcpkgDir = $(Get-Location)
    [Environment]::SetEnvironmentVariable("VCPKGDir", $env:vcpkgDir,
        [EnvironmentVariableTarget]::User)
    echo "vcpkg already installed locally, updating"
    # Update and rebuild vcpkg
    git pull
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
$targetTriplet = "x64-windows-static"
vcpkg --triplet $targetTriplet install eigen3 fltk zlib libflac libogg libvorbis libsndfile libsamplerate portmidi portaudio liblo hdf5 dirent

$vcpkgTiming = (Get-Date).TimeOfDay

# Comment for testing to avoid extracting if already done so
rm -Path deps -Force -Recurse -ErrorAction SilentlyContinue
mkdir cache -ErrorAction SilentlyContinue
mkdir deps -ErrorAction SilentlyContinue
mkdir $depsLibDir -ErrorAction SilentlyContinue
mkdir $depsBinDir -ErrorAction SilentlyContinue
mkdir $depsIncDir -ErrorAction SilentlyContinue
mkdir staging -ErrorAction SilentlyContinue

echo "Downloading and installing non-VCPKG packages..."

choco install swig -y
choco upgrade swig -y

choco install winflexbison -y
choco upgrade winflexbison -y

# List of URIs to download and install
$uriList=
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/dependencies/gettext-runtime_0.18.1.1-2_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/dependencies/pkg-config_0.23-2_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/dependencies/proxy-libintl-dev_20100902_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/glib/2.26/glib-dev_2.26.1-1_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/glib/2.26/glib_2.26.1-1_win64.zip",
"http://download-mirror.savannah.gnu.org/releases/getfem/stable/gmm-5.1.tar.gz",
"https://github.com/thestk/stk/archive/master.zip"

# Appends this folder location to the 'deps' uri
$destList=
"fluidsynthdeps",
"fluidsynthdeps",
"fluidsynthdeps",
"fluidsynthdeps",
"fluidsynthdeps",
"",
""

# Download list of files to cache folder
for($i=0; $i -lt $uriList.Length; $i++)
{
    $fileName = Split-Path -Leaf $uriList[$i]
    $cachedFile = $cacheDir + $fileName
    if (Test-Path $cachedFile -PathType Leaf)
    {
        echo "Already downloaded file: $fileName"
    }
    else
    {
      echo "Downloading: " $uriList[$i]
      $webclient.DownloadFile($uriList[$i], $cachedFile)
    }
}

# Extract libs to deps directory
for($i=0; $i -lt $uriList.Length; $i++)
{
    $fileName = Split-Path -Leaf $uriList[$i]
    $cachedFile = $cacheDir + $fileName
    $destDir = $depsDir + $destList[$i]
    if ($PSVersionTable.PSVersion.Major -gt 3)
    {
        Expand-Archive $cachedFile -DestinationPath $destDir -Force
    }
    else
    {
        New-Item $destDir -ItemType directory -Force
        Expand-Archive $cachedFile -OutputPath $destDir -Force
    }
    echo "Extracted $fileName to $destDir"
}

# STK
cd $depsDir

Copy-Item ($destDir + "stk-master\*") ($csoundDir + "\Opcodes\stk") -recurse -force
echo "STK: Copied sources to Csound opcodes directory."

# GMM
cd $cacheDir
7z e -y "gmm-5.1.tar.gz"
7z x -y "gmm-5.1.tar"
cd ..
copy ($cacheDir + "gmm-5.1\include\gmm\") -Destination ($depsIncDir + "gmm\") -Force -Recurse
echo "Copied v5.1 gmm headers to deps include directory. Please note, verson 5.1 is REQUIRED, "
echo "later versions do not function as stand-alone, header-file-only libraries."

echo "FluidSynth..."
cd $stageDir

if (Test-Path "fluidsynth")
{
    cd fluidsynth
    git pull
    cd ..
    echo "Fluidsynth already downloaded, updated"
}
else
{
    git clone --depth=1 -b v1.1.10 "https://github.com/FluidSynth/fluidsynth.git"
}

rm -Path fluidsynthbuild -Force -Recurse -ErrorAction SilentlyContinue
mkdir fluidsynthbuild -ErrorAction SilentlyContinue
cd fluidsynthbuild
cmake ..\fluidsynth -G $vsGenerator -T $vsToolset -DCMAKE_PREFIX_PATH="$depsDir\fluidsynthdeps" -DCMAKE_INCLUDE_PATH="$depsDir\fluidsynthdeps\include\glib-2.0;$depsDir\fluidsynthdeps\lib\glib-2.0\include"
cmake --build . --config Release
copy .\src\Release\fluidsynth.exe -Destination $depsBinDir -Force
copy .\src\Release\fluidsynth.lib -Destination $depsLibDir -Force
copy .\src\Release\libfluidsynth-1.dll -Destination $depsBinDir -Force
copy .\include\fluidsynth.h -Destination $depsIncDir -Force
robocopy ..\fluidsynth\include\fluidsynth $depsIncDir\fluidsynth *.h /s /NJH /NJS
copy .\include\fluidsynth\version.h -Destination $depsIncDir\fluidsynth\version.h -Force

$buildTiming = (Get-Date).TimeOfDay

# Add deps bin directory to the system path if not already there
# FIXME this is duplicating part of the path for some reason
if ($env:Path.Contains($depsBinDir))
{
    echo "Already added dependency bin dir to path"
}
else
{
    # For this session add to path var
    $env:Path += ";" + $depsBinDir

    # Permanently add to system path
    [Environment]::SetEnvironmentVariable("Path", $env:Path, [EnvironmentVariableTarget]::User)
    echo "Added dependency bin dir to path: $depsBinDir"
}

$endTime = (Get-Date).TimeOfDay
$buildTiming = $buildTiming - $vcpkgTiming
$vcpkgTiming = $vcpkgTiming - $startTime
$duration = $endTime - $startTime

# Strip any unneeded files from the bin directory, this will be
# copied directly into the Csound solution output directory
cd $depsBinDir
rm *.exe

echo "Removed unnecessary files from dependency bin directory"
echo " "
echo "VCPKG duration: $($vcpkgTiming.TotalMinutes) minutes"
echo "Build duration: $($buildTiming.TotalMinutes) minutes"
echo "Total duration: $($duration.TotalMinutes) minutes"
