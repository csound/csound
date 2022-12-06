$ErrorActionPreference = 'Stop';
$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$fileLocation = Join-Path $toolsDir 'csound_installer.exe'

$packageArgs = @{
  packageName   = $env:ChocolateyPackageName
  unzipLocation = $toolsDir
  fileType      = 'exe'
  file          = $fileLocation

  softwareName  = 'csound*'

  validExitCodes= @(0)
  silentArgs = '/sp- /silent /norestart'
}

Install-ChocolateyInstallPackage @packageArgs
