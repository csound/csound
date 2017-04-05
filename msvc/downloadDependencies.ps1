echo "Downloading Csound dependencies..."

$webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path

# Testing only, remove all files 
rm -Path cache -Force -Recurse
rm -Path deps -Force -Recurse
rm -Path vcpkg -Force -Recurse

$vcpkgDir = $currentDir + "\vcpkg"

# Download vcpkg if it doesn't exist
if (Test-Path $vcpkgDir)
{
    echo "vcpkg already installed"
}
else {
    git clone --depth 1 http://github.com/Microsoft/vcpkg.git
    cd vcpkg
    powershell -exec bypass scripts\bootstrap.ps1
    cd ..
}

# can be arm-uwp, x64-uwp, x64-windows-static, x64-windows, x86-uwp, x86-windows-static, x86-windows
$targetTriplet = "x64-windows"
$vcPackages = "boost", "curl", "eigen3", "fltk", "gtk", "libflac", 
"lua", "libogg", "portaudio", "libvorbis", "zlib"

for($i=0; $i -lt $vcPackages.Length; $i++) {
    $vcpkgDeps64 = $vcpkgDeps64 + " " + $vcPackages[$i] + ":" + $targetTriplet
}

echo "Downloading VC packages..."
echo $vcpkgDeps64

# Download vcpkg dependencies
.\vcpkg\vcpkg.exe install $vcpkgDeps64

mkdir cache -ErrorAction SilentlyContinue
mkdir deps -ErrorAction SilentlyContinue

# Doesn't work with sourceforge due to re-direct behavior
#curl -OutFile cache/libsndfile-1.0.27-w64.zip -Uri http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip -ErrorAction SilentlyContinue
#curl -OutFile cache/win_flex_bison-latest.zip -Uri https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip -ErrorAction SilentlyContinue -UserAgent ([Microsoft.PowerShell.Commands.PSUserAgent]::Chrome)

# List of URIs to download and install
$uriList="http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip",
"https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip"

#Appends this folder location to the 'deps' uri
$destList="", 
"win_flex_bison"

$cacheDir = $currentDir + "\cache\"
$depsDir = $currentDir + "\deps\"

for($i=0; $i -lt $uriList.Length; $i++) {
    $fileName = Split-Path -Leaf $uriList[$i]
    $cachedFile = $cacheDir + $fileName
    echo "Download.." $uriList[$i] " to " $cachedFile
    $webclient.DownloadFile($uriList[$i], $cachedFile)
    $destDir = $depsDir + $destList[$i]
    Expand-Archive $cachedFile -DestinationPath $destDir
}
