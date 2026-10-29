param(
    [Parameter(Mandatory = $true)]
    [uri] $Uri,
    [Parameter(Mandatory = $true)]
    [string] $Destination
)

$ErrorActionPreference = 'Stop'
$downloadPath = "$Destination.download.exe"

for ($attempt = 1; $attempt -le 3; $attempt++) {
    try {
        Write-Host "Downloading Visual C++ Redistributable from $Uri (attempt $attempt of 3)"
        # A truncated CDN response can complete without a download error.
        # Request a fresh response and validate it before replacing the installer input.
        Invoke-WebRequest -Uri $Uri -OutFile $downloadPath -UseBasicParsing -TimeoutSec 120 `
            -Headers @{ 'Cache-Control' = 'no-cache'; 'Pragma' = 'no-cache' }

        $hash = (Get-FileHash -LiteralPath $downloadPath -Algorithm SHA256).Hash
        $size = (Get-Item -LiteralPath $downloadPath).Length
        Write-Host "Downloaded runtime - SHA256: $hash Size: $size bytes"
        $signature = Get-AuthenticodeSignature -LiteralPath $downloadPath
        if ($signature.Status -ne 'Valid') {
            throw "Authenticode signature invalid: $($signature.Status) $($signature.StatusMessage)"
        }
        if ($signature.SignerCertificate.Subject -notmatch '(^|,\s*)O=Microsoft Corporation(,|$)') {
            throw "Unexpected signer: $($signature.SignerCertificate.Subject)"
        }

        Write-Host "Authenticode valid - Subject: $($signature.SignerCertificate.Subject) Issuer: $($signature.SignerCertificate.Issuer)"
        Move-Item -LiteralPath $downloadPath -Destination $Destination -Force
        return
    }
    catch {
        Remove-Item -LiteralPath $downloadPath -Force -ErrorAction SilentlyContinue
        if ($attempt -eq 3) {
            throw
        }
        Write-Warning "Runtime download failed validation or transfer: $($_.Exception.Message). Retrying."
        Start-Sleep -Seconds (5 * $attempt)
    }
}
