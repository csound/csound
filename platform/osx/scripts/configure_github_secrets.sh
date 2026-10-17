
#!/bin/bash
set -euo pipefail

REPO="${REPO:-csound/csound}"
CERT_PASSWORD="test123"

read -rsp "Certificate password [$CERT_PASSWORD]: " INPUT_PASSWORD
echo ""
CERT_PASSWORD="${INPUT_PASSWORD:-$CERT_PASSWORD}"

source "$(dirname "$0")/print_github_secrets.sh"

echo "Configuring GitHub repository: $REPO"

# GitHub Actions variables
gh variable set DEVELOPER_APP_ID \
    --repo "$REPO" \
    --body "$DEVELOPER_APP_ID"

gh variable set DEVELOPER_INSTALLER_ID \
    --repo "$REPO" \
    --body "$DEVELOPER_INSTALLER_ID"

# GitHub Actions secrets
gh secret set DEV_ID_APP_CERT \
    --repo "$REPO" \
    --body "$DEV_ID_APP_CERT"

gh secret set DEV_ID_APP_PASSWORD \
    --repo "$REPO" \
    --body "$CERT_PASSWORD"

gh secret set DEV_ID_INSTALLER_CERT \
    --repo "$REPO" \
    --body "$DEV_ID_INSTALLER_CERT"

gh secret set DEV_ID_INSTALLER_PASSWORD \
    --repo "$REPO" \
    --body "$CERT_PASSWORD"

echo "GitHub Actions variables and secrets configured for $REPO."
