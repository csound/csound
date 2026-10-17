#!/bin/bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

DEVELOPER_APP_ID="CsoundTestApplication"
DEVELOPER_INSTALLER_ID="CsoundTestInstaller"

DEV_ID_APP_CERT=$(base64 -i certificates/app.p12 | tr -d '\n')
DEV_ID_INSTALLER_CERT=$(base64 -i certificates/installer.p12 | tr -d '\n')

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "DEVELOPER_APP_ID=$DEVELOPER_APP_ID"
    echo "DEVELOPER_INSTALLER_ID=$DEVELOPER_INSTALLER_ID"
    echo ""
    echo "DEV_ID_APP_CERT=$DEV_ID_APP_CERT"
    echo ""
    echo "DEV_ID_INSTALLER_CERT=$DEV_ID_INSTALLER_CERT"
fi
