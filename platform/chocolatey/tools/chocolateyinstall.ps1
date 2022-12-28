$ErrorActionPreference = 'Stop';
$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$fileLocation = Join-Path $toolsDir 'csound-windows_x86_64-6.17.0-916.exe'

$packageArgs = @{
  packageName   = $env:ChocolateyPackageName
  unzipLocation = $toolsDir
  fileType      = 'exe'
  file          = $fileLocation

  softwareName  = 'csound*'

  validExitCodes= @(0)
  silentArgs = '/sp- /verysilent /norestart'
}

Install-ChocolateyInstallPackage @packageArgs
