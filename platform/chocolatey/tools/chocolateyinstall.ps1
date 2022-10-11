$ErrorActionPreference = 'Stop';
$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$fileLocation = Join-Path $toolsDir 'csound_installer.exe'

$packageArgs = @{
  packageName   = $env:ChocolateyPackageName
  unzipLocation = $toolsDir
  fileType      = 'exe'
  file          = $fileLocation

  softwareName  = 'csound*'

  checksum64     = '68ff3629a20fe91385c6bb8660eb95f8df5ecb6619d1d0becfa6bf4c9748435b'
  checksumType64 = 'sha256'

  validExitCodes= @(0)
  silentArgs = '/sp- /silent /norestart'
}

Install-ChocolateyInstallPackage @packageArgs
