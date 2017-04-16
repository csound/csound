echo "Downloading Csound dependencies..."

$webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path
$cacheDir = $currentDir + "\cache\"
$depsDir = $currentDir + "\deps\"
$stageDir = $currentDir + "\staging\"
$depsBinDir = $depsDir + "bin\"
$vcpkgDir = $currentDir + "\vcpkg"

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
    git pull
    vcpkg remove --outdated --recurse
    vcpkg update
    cd $currentDir
}
elseif (Test-Path $vcpkgDir)
{
    $env:Path += ";" + $vcpkgDir
    echo "vcpkg already installed locally, updating"
    cd vcpkg
    git pull
    vcpkg remove --outdated --recurse
    vcpkg update
    cd ..
}
else {
    $env:Path += ";" + $vcpkgDir
    echo "vcpkg missing, downloading and installing"
    git clone --depth 1 http://github.com/Microsoft/vcpkg.git
    cd vcpkg
    powershell -exec bypass scripts\bootstrap.ps1
    vcpkg integrate install
    cd ..
}

# Download all vcpkg packages available
# Target can be arm-uwp, x64-uwp, x64-windows-static, x64-windows, x86-uwp, x86-windows-static, x86-windows
$targetTriplet = "x64-windows"
$vcPackages = "boost", "curl", "eigen3", "fltk", "gtk", "libflac", 
"lua", "libogg", "libvorbis", "zlib"

echo "Downloading VC packages..."

for($i=0; $i -lt $vcPackages.Length; $i++) {
    vcpkg --triplet $targetTriplet install $vcPackages[$i]
}

rm -Path deps -Force -Recurse -ErrorAction SilentlyContinue
mkdir cache -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
mkdir deps -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
mkdir staging -InformationAction SilentlyContinue -ErrorAction SilentlyContinue

# Manual packages to download and install
# List of URIs to download and install
$uriList="http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip",
"https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip",
"http://www.steinberg.net/sdk_downloads/asiosdk2.3.zip"

# Appends this folder location to the 'deps' uri
$destList="", 
"win_flex_bison"

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
# portaudio
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

mkdir portaudioBuild -InformationAction SilentlyContinue -ErrorAction SilentlyContinue
cd portaudioBuild 
cmake ..\portaudio -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE="Release" -DPA_USE_ASIO=1
cmake --build . --config Release
copy .\Release\portaudio_x64.dll -Destination $depsBinDir

# Add deps bin directory to the system path if not already there
if ($env:Path.Contains($depsBinDir))
{
    echo "Already added dependency bin dir to path"
}
else
{
    # Permanently add to system path
    [Environment]::SetEnvironmentVariable("Path", $env:Path + ";" + $depsBinDir, 
        [EnvironmentVariableTarget]::User)

    # For this session add to path var
    $env:Path += ";" + $depsBinDir

    echo "Added dependency bin dir to path: $depsBinDir" 
}

# Generate solution file
cd $currentDir
mkdir csound-vs -ErrorAction SilentlyContinue
cd csound-vs
echo "Generating Csound VS project..."

cmake ..\.. -G "Visual Studio 15 2017 Win64" `
 -Wdev -Wdeprecated `
 -DCMAKE_BUILD_TYPE="Release" `
 -DCMAKE_TOOLCHAIN_FILE="$vcpkgDir\scripts\buildsystems\vcpkg.cmake" `
 -DCMAKE_INSTALL_PREFIX=dist `
 -DCUSTOM_CMAKE="..\Custom-vs.cmake" `
 -DHAVE_BIG_ENDIAN=0 `
 -DCMAKE_16BIT_TYPE="unsigned short" `
 -DUSE_DOUBLE=1 `
 -DUSE_GETTEXT=0 `
 -DUSE_CURL=0 `
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

echo "Finished"
