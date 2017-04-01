echo "Downloading Csound dependencies..."

# Testing only
rm -Path cache -Force -Recurse
rm -Path deps -Force -Recurse

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

$webclient = New-Object System.Net.WebClient
$currentDir = Split-Path $MyInvocation.MyCommand.Path
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
