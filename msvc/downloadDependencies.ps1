param
(
    [string]$vsGenerator="Visual Studio 15 2017 Win64",
    [string]$vsToolset="v141"
)

echo "Downloading Csound dependencies..."
echo "vsGenerator: $vsGenerator"
echo "vsToolset:   $vsToolset"

$startTime = (Get-Date).TimeOfDay

# Add different protocols to get download working for HDF5 site
# ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12 | SecurityProtocolType.Ssl3;
[System.Net.ServicePointManager]::SecurityProtocol =  [System.Net.SecurityProtocolType]::Tls12;

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
$env:Path += $depsDir

# Find VCPKG from path if it already exists
# Otherwise use the local Csound version that will be installed
$systemVCPKG = $(Get-Command vcpkg -ErrorAction SilentlyContinue).Source

# Test if VCPKG is already installed on system
# Download locally to csound msvc folder if not
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
    [Environment]::SetEnvironmentVariable("VCPKGDir", $env:vcpkgDir,
        [EnvironmentVariableTarget]::User)
    powershell -exec bypass scripts\bootstrap.ps1
    vcpkg integrate install
    cd $currentDir
}

# Generate VCPKG AlwaysAllowDownloads file if needed
New-Item -type file $vcpkgDir\downloads\AlwaysAllowDownloads -errorAction SilentlyContinue | Out-Null

# Download all vcpkg packages available
echo "Downloading VC packages..."
# Target can be arm-uwp, x64-uwp, x64-windows-static, x64-windows, x86-uwp, x86-windows-static, x86-windows
$targetTriplet = "x64-windows"
$targetTripletStatic = "x64-windows-static"
#vcpkg --triplet $targetTriplet install eigen3 fltk zlib 
#vcpkg --triplet $targetTripletStatic install libflac libogg libvorbis libsndfile
vcpkg --triplet $targetTripletStatic install eigen3 fltk zlib libflac libogg libvorbis libsndfile libsamplerate
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

# List of URIs to download and install
$uriList="https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip",
"http://www.steinberg.net/sdk_downloads/asiosdk2.3.zip",
"https://downloads.sourceforge.net/project/swig/swigwin/swigwin-3.0.12/swigwin-3.0.12.zip",
"http://www.steinberg.net/sdk_downloads/vstsdk367_03_03_2017_build_352.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/dependencies/gettext-runtime_0.18.1.1-2_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/dependencies/pkg-config_0.23-2_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/dependencies/proxy-libintl-dev_20100902_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/glib/2.26/glib-dev_2.26.1-1_win64.zip",
"http://ftp.acc.umu.se/pub/gnome/binaries/win64/glib/2.26/glib_2.26.1-1_win64.zip",
"http://download-mirror.savannah.gnu.org/releases/getfem/stable/gmm-5.1.tar.gz",
"https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.19/bin/windows/hdf5-1.8.19-Std-win10_64-vs2015.zip",
"https://github.com/thestk/stk/archive/master.zip"

# commenting out 1.8.20 for now
#"https://support.hdfgroup.org/ftp/HDF5/current18/bin/windows/hdf5-1.8.20-Std-win7_64-vs14.zip",

# Appends this folder location to the 'deps' uri
$destList="win_flex_bison",
"",
"",
"",
"fluidsynthdeps",
"fluidsynthdeps",
"fluidsynthdeps",
"fluidsynthdeps",
"fluidsynthdeps",
"",
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

# Ableton
cd $depsDir
echo "Ableton Link..."
if (Test-Path "link")
{
   cd link
   git pull
   git submodule update --recursive
   echo "Ableton Link already downloaded, updated."
}
else
{
   git clone --branch Link-3.0.0 https://github.com/Ableton/link.git
   cd link
   git submodule update --init --recursive
   echo "Ableton Link downloaded."
}

mkdir build
cd build
cmake .. -G $vsGenerator -T $vsToolset -DCMAKE_BUILD_TYPE="Release"
cmake --build .

# disable 1.8.20 for time being
# cd $depsDir    
# cd hdf5-1.8.20-Std-win7_64-vs14
# dir hdf   
# Start-Process msiexec -Wait -ArgumentList '/I hdf\HDF5-1.8.20-win64.msi /quiet /qn /li /norestart'   
# echo "Installed HDF5..."    

cd $depsDir    
dir hdf   
Start-Process msiexec -Wait -ArgumentList '/I hdf\HDF5-1.8.19-win64.msi /quiet /qn /li /norestart'   
echo "Installed HDF5..."    


cd $depsDir

Copy-Item ($destDir + "stk-master\*") ($csoundDir + "\Opcodes\stk") -recurse -force
echo "STK: Copied sources to Csound opcodes directory."

cd $cacheDir
7z e -y "gmm-5.1.tar.gz"
7z x -y "gmm-5.1.tar"
cd ..
copy ($cacheDir + "gmm-5.1\include\gmm\") -Destination ($depsIncDir + "gmm\") -Force -Recurse
echo "Copied v5.1 gmm headers to deps include directory. Please note, verson 5.1 is REQUIRED, "
echo "later versions do not function as stand-alone, header-file-only libraries."

cd $stageDir
copy ..\deps\ASIOSDK2.3 -Destination . -Recurse -ErrorAction SilentlyContinue -Force
echo "ASIOSDK2.3: Copied sources to deps."

echo "PortAudio..."
if (Test-Path "portaudio")
{
    cd portaudio
    git pull
    cd ..
    echo "Portaudio already downloaded, updated."
}
else
{
    git clone --depth=1 "https://git.assembla.com/portaudio.git"
}

copy portaudio\include\portaudio.h -Destination $depsIncDir -Force
rm -Path portaudioBuild -Force -Recurse -ErrorAction SilentlyContinue
mkdir portaudioBuild -ErrorAction SilentlyContinue
cd portaudioBuild
cmake ..\portaudio -G $vsGenerator -T $vsToolset -DCMAKE_BUILD_TYPE="Release" -DPA_USE_ASIO=1
cmake --build . --config Release
copy .\Release\portaudio_x64.dll -Destination $depsBinDir -Force
copy .\Release\portaudio_x64.lib -Destination $depsLibDir -Force

echo "PortMidi..."
cd $stageDir

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
cmake .. -G $vsGenerator -T $vsToolset -DCMAKE_BUILD_TYPE="Release"
cmake --build . --config Release
copy .\Release\portmidi.dll -Destination $depsBinDir -Force
copy .\Release\portmidi.lib -Destination $depsLibDir -Force
copy .\Release\portmidi_s.lib -Destination $depsLibDir -Force
copy .\Release\pmjni.dll -Destination $depsBinDir -Force
copy .\Release\pmjni.lib -Destination $depsLibDir -Force
copy ..\pm_common\portmidi.h -Destination $depsIncDir -Force
copy ..\porttime\porttime.h -Destination $depsIncDir -Force

echo "LibLo..."
cd $stageDir

if (Test-Path "liblo")
{
    cd liblo
    git pull
    cd ..
    echo "Libo already downloaded, updated"
}
else
{
    git clone --depth=1 "https://github.com/radarsat1/liblo.git"
}

rm -Path liblo\cmakebuild -Force -Recurse -ErrorAction SilentlyContinue
mkdir liblo\cmakebuild -ErrorAction SilentlyContinue
cd liblo\cmakebuild
cmake ..\cmake -G $vsGenerator -T $vsToolset -DCMAKE_BUILD_TYPE="Release" -DTHREADING=1
cmake --build . --config Release
copy .\Release\lo.dll -Destination $depsBinDir -Force
copy .\Release\lo.lib -Destination $depsLibDir -Force
copy .\lo -Destination $depsIncDir -Force -Recurse
copy ..\lo\* -Destination $depsIncDir\lo -Force -Include "*.h"
robocopy ..\lo $depsIncDir\lo *.h /s /NJH /NJS

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
    #Switch to offical branch when PR is merged in
    git clone --depth=1 -b master "https://github.com/stekyne/fluidsynth.git"
}

rm -Path fluidsynthbuild -Force -Recurse -ErrorAction SilentlyContinue
mkdir fluidsynthbuild -ErrorAction SilentlyContinue
cd fluidsynthbuild
cmake ..\fluidsynth -G $vsGenerator -T $vsToolset -DCMAKE_PREFIX_PATH="$depsDir\fluidsynthdeps" -DCMAKE_INCLUDE_PATH="$depsDir\fluidsynthdeps\include\glib-2.0;$depsDir\fluidsynthdeps\lib\glib-2.0\include"
cmake --build . --config Release
copy .\src\Release\fluidsynth.exe -Destination $depsBinDir -Force
copy .\src\Release\fluidsynth.lib -Destination $depsLibDir -Force
copy .\src\Release\libfluidsynth-2.dll -Destination $depsBinDir -Force
copy ..\fluidsynth\include\fluidsynth.h -Destination $depsIncDir -Force
robocopy ..\fluidsynth\include\fluidsynth $depsIncDir\fluidsynth *.h /s /NJH /NJS
copy .\include\fluidsynth\version.h -Destination $depsIncDir\fluidsynth -Force

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
    [Environment]::SetEnvironmentVariable("Path", $env:Path,
        [EnvironmentVariableTarget]::User)

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
