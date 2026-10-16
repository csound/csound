#!/bin/bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

umask 077

CERT_PASSWORD="${CERT_PASSWORD:-test123}"

mkdir -p certificates
cd certificates

create_cert() {
    local name="$1"
    local prefix="$2"
    local extension="$3"

    openssl req -x509 -newkey rsa:2048 \
        -keyout "$prefix.key" \
        -out "$prefix.crt" \
        -days 365 -nodes \
        -subj "/CN=$name" \
        -addext "keyUsage=critical,digitalSignature" \
        -addext "$extension"

    openssl pkcs12 -export \
        -inkey "$prefix.key" \
        -in "$prefix.crt" \
        -out "$prefix.p12" \
        -passout "pass:$CERT_PASSWORD"
}

create_cert \
    "CsoundTestApplication" \
    "app" \
    "extendedKeyUsage=codeSigning"

create_cert \
    "CsoundTestInstaller" \
    "installer" \
    "extendedKeyUsage=1.2.840.113635.100.4.13"

echo "Certificates generated in certificates/"
