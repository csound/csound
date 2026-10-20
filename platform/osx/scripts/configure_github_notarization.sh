#!/bin/bash
set -euo pipefail

REPO="${REPO:-csound/csound}"

APPLE_ID=appleId
APPLE_TEAM_ID=appleTeamId

read -rsp "Apple App password: " APPLE_APP_PASSWORD
echo

[[ -n "$APPLE_APP_PASSWORD" ]] || {
    echo "Apple App password is required" >&2
    exit 1
}

# GitHub Actions secrets
gh secret set APPLE_ID \
    --repo "$REPO" \
    --body "$APPLE_ID"

gh secret set APPLE_TEAM_ID \
    --repo "$REPO" \
    --body "$APPLE_TEAM_ID"

gh secret set APPLE_APP_PASSWORD \
    --repo "$REPO" \
    --body "$APPLE_APP_PASSWORD"

# GitHub Actions variables
gh variable set ENABLE_NOTARIZATION \
    --repo "$REPO" \
    --body "true"

echo "GitHub secrets configured for $REPO."
