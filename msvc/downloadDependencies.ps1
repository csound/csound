echo "Downloading Csound dependencies..."

$startTime = (Get-Date).TimeOfDay

$webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path
$cacheDir = $currentDir + "\cache\"
$depsDir = $currentDir + "\deps\"
$stageDir = $currentDir + "\staging\"
$depsBinDir = $depsDir + "bin\"
$depsLibDir = $depsDir + "lib\"
$depsIncDir = $depsDir + "include\"
$vcpkgDir = ""

# Add to path to call premake or other tools
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
    echo "vcpkg missing, downloading and installing"

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

# Download all vcpkg packages available
# Target can be arm-uwp, x64-uwp, x64-windows-static, x64-windows, x86-uwp, x86-windows-static, x86-windows
$targetTriplet = "x64-windows"
echo "Downloading VC packages..."

vcpkg --triplet $targetTriplet install curl eigen3 fltk libflac lua libogg libvorbis zlib

# Comment for testing to avoid extracting if already done so
rm -Path deps -Force -Recurse -ErrorAction SilentlyContinue
mkdir cache -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
mkdir deps -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
mkdir staging -InformationAction SilentlyContinue -ErrorAction SilentlyContinue

# Manual packages to download and install
# List of URIs to download and install
$uriList="http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip",
"https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip",
"http://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"
#"http://www.steinberg.net/sdk_downloads/vstsdk367_03_03_2017_build_352.zip"

# Appends this folder location to the 'deps' uri
$destList="", 
"win_flex_bison",
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
    Expand-Archive $cachedFile -DestinationPath $destDir -Force
    echo "Extracted $fileName"
}

# Manual building...
# Portaudio
cd staging
copy ..\deps\ASIOSDK2.3 -Destination . -Recurse -ErrorAction SilentlyContinue

if (Test-Path "portaudio")
{
    cd portaudio
    git pull
    cd ..
    echo "Portaudio already downloaded, updated"
}
else
{
    git clone --depth=1 "https://git.assembla.com/portaudio.git"
}

copy portaudio\include\portaudio.h -Destination $depsIncDir -Force
mkdir portaudioBuild -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
cd portaudioBuild 
cmake ..\portaudio -G "Visual Studio 14 2015 Win64" -DCMAKE_BUILD_TYPE="Release" -DPA_USE_ASIO=1
cmake --build . --config Release
copy .\Release\portaudio_x64.dll -Destination $depsBinDir -Force
copy .\Release\portaudio_x64.lib -Destination $depsLibDir -Force

# Liblo
cd ..
# TEMP: seeing as the repo has changed, need to delete the old one first
Remove-Item -Recurse -Force liblo

if (Test-Path "liblo")
{
    cd liblo
    git pull
    cd ..
    echo "Libo already downloaded, updated"
}
else
{
    git clone --depth=1 "https://github.com/stekyne/liblo.git"
}

mkdir liblo\cmakebuild -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
cd liblo\cmakebuild
cmake ..\cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE="Release" -DTHREADING=1
cmake --build . --config Release
copy .\Release\lo.dll -Destination $depsBinDir -Force
copy .\Release\lo.lib -Destination $depsLibDir -Force
copy .\lo -Destination $depsIncDir -Force -Recurse
copy ..\lo\* -Destination $depsIncDir\lo -Force -Include "*.h"
robocopy ..\lo $depsIncDir\lo *.h /s /NJH /NJS

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
$duration = $endTime - $startTime

# Strip any unneeded files from the bin directory, this will be
# copied directly into the Csound solution output directory
cd $depsBinDir 
rm *.exe
cd $currentDir
echo "Removed unnecessary files from dependency bin directory"
echo "Finished in $($duration.TotalMinutes) minutes"
