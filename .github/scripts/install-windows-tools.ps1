$ErrorActionPreference = 'Stop'
# Check native exit codes ourselves so a failed download can be retried.
$PSNativeCommandUseErrorActionPreference = $false

function Test-BuildTools($Tools) {
    foreach ($name in $Tools.Keys) {
        $command = Get-Command $name -CommandType Application -ErrorAction SilentlyContinue
        if (!$command) {
            Write-Host "Missing build tool $name"
            return $false
        }
        & $command.Source $Tools[$name] | Out-Host
        if ($LASTEXITCODE -ne 0) {
            Write-Host "$name failed with exit code $LASTEXITCODE"
            return $false
        }
    }
    return $true
}

$packages = @(
    @{ Name = 'winflexbison3'; Tools = @{ 'win_flex.exe' = '--version'; 'win_bison.exe' = '--version' } },
    @{ Name = 'innosetup'; Tools = @{ 'iscc.exe' = '/?' } }
)

foreach ($package in $packages) {
    if (!(Test-BuildTools $package.Tools)) {
        for ($attempt = 1; $attempt -le 3; $attempt++) {
            Write-Host "Installing $($package.Name), attempt $attempt of 3"
            choco install $package.Name -y --no-progress
            $installExitCode = $LASTEXITCODE

            # Keep paths added by earlier workflow steps and pick up new installs.
            $paths = @($env:Path -split [IO.Path]::PathSeparator) +
                @([Environment]::GetEnvironmentVariable('Path', 'Machine') -split [IO.Path]::PathSeparator) +
                @([Environment]::GetEnvironmentVariable('Path', 'User') -split [IO.Path]::PathSeparator)
            $env:Path = ($paths | Where-Object { $_ } | Select-Object -Unique) -join [IO.Path]::PathSeparator

            # Chocolatey can report success even when a feed request failed.
            # Require both a successful install and usable tools before building.
            $toolsReady = Test-BuildTools $package.Tools
            if (($installExitCode -in 0, 1641, 3010) -and $toolsReady) {
                break
            }
            if ($attempt -eq 3) {
                throw "Could not install usable tools for $($package.Name) after 3 attempts (Chocolatey exit code $installExitCode)."
            }
            Write-Warning "Could not prepare $($package.Name) (Chocolatey exit code $installExitCode). Retrying."
            Start-Sleep -Seconds (15 * $attempt)
        }
    }

    # Make freshly installed tools available to later workflow steps too.
    if ($env:GITHUB_PATH) {
        foreach ($name in $package.Tools.Keys) {
            $directory = Split-Path (Get-Command $name -CommandType Application).Source
            Add-Content -Path $env:GITHUB_PATH -Value $directory -Encoding utf8
        }
    }
}

# Normalize Chocolatey's successful reboot codes for the workflow runner.
exit 0
