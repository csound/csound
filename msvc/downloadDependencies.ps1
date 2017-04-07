echo "Downloading Csound dependencies..."

$webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path
$cacheDir = $currentDir + "\cache\"
$depsDir = $currentDir + "\deps\"
$vcpkgDir = $currentDir + "\vcpkg"

# Testing only, remove all files 
#rm -Path cache -Force -Recurse -ErrorAction SilentlyContinue
#rm -Path deps -Force -Recurse -ErrorAction SilentlyContinue
#rm -Path vcpkg -Force -Recurse -ErrorAction SilentlyContinue

# Download vcpkg if it doesn't exist
if (Test-Path $vcpkgDir)
{
    echo "vcpkg already installed, updating"
    cd vcpkg
    git pull
    cd ..
}
else {
    echo "vcpkg missing, downloading and installing"
    git clone --depth 1 http://github.com/Microsoft/vcpkg.git
    cd vcpkg
    powershell -exec bypass scripts\bootstrap.ps1
    cd ..
}

# Download all vcpkg packages available
# Target can be arm-uwp, x64-uwp, x64-windows-static, x64-windows, x86-uwp, x86-windows-static, x86-windows
$targetTriplet = "x64-windows"
$vcPackages = "boost", "curl", "eigen3", "fltk", "gtk", "libflac", 
"lua", "libogg", "portaudio", "libvorbis", "zlib"

echo "Downloading VC packages..."

for($i=0; $i -lt $vcPackages.Length; $i++) {
    .\vcpkg\vcpkg.exe --triplet $targetTriplet install $vcPackages[$i]
}

mkdir cache -ErrorAction SilentlyContinue
mkdir deps -ErrorAction SilentlyContinue

# Doesn't work with sourceforge due to re-direct behavior
#curl -OutFile cache/libsndfile-1.0.27-w64.zip -Uri http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip -ErrorAction SilentlyContinue
#curl -OutFile cache/win_flex_bison-latest.zip -Uri https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip -ErrorAction SilentlyContinue -UserAgent ([Microsoft.PowerShell.Commands.PSUserAgent]::Chrome)

# Manual packages to download and install
# List of URIs to download and install
$uriList="http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip",
"https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip"

# Appends this folder location to the 'deps' uri
$destList="", 
"win_flex_bison"

for($i=0; $i -lt $uriList.Length; $i++) 
{
    $fileName = Split-Path -Leaf $uriList[$i]
    $cachedFile = $cacheDir + $fileName
    if (Test-Path $cachedFile -PathType Leaf)
    {
        echo "Already downloaded file: $fileName, skipping"
    }
    else 
    {
    	echo "Downloading (build and install): " $uriList[$i]
    	$webclient.DownloadFile($uriList[$i], $cachedFile)
    }
    $destDir = $depsDir + $destList[$i]
    Expand-Archive $cachedFile -DestinationPath $destDir -Force
    echo "Extracted $fileName"
}

echo "Finished"
