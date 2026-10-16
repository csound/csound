# macOS Signing Certificates

This guide explains how to configure GitHub Actions to sign Csound macOS releases.

## 1. Prepare certificates

Copy the existing Apple Developer `.p12` certificates into the `certificates/` directory:

```bash
cp /path/to/app.p12 certificates/app.p12
cp /path/to/installer.p12 certificates/installer.p12
```

Update `DEVELOPER_APP_ID` and `DEVELOPER_INSTALLER_ID` in `print_github_secrets.sh` to match the certificate identities.

**Note:** The `certificates/` directory contains sensitive files and must not be committed to Git.

## 2. Install GitHub CLI

```bash
brew install gh
```

## 3. Authenticate with GitHub

```bash
gh auth login
```

Make sure your GitHub account has permission to configure Actions secrets and variables in the target repository.

## 4. Configure GitHub Actions

Run:

```bash
./configure_github_secrets.sh
```

The script prompts for the certificate password and configures the following:

**GitHub Actions variables:**
- `DEVELOPER_APP_ID`
- `DEVELOPER_INSTALLER_ID`

**GitHub Actions secrets:**
- `DEV_ID_APP_CERT`
- `DEV_ID_APP_PASSWORD`
- `DEV_ID_INSTALLER_CERT`
- `DEV_ID_INSTALLER_PASSWORD`

Verify the configuration:

```bash
gh variable list --repo OWNER/REPO
gh secret list --repo OWNER/REPO
```

Replace `OWNER/REPO` with the target GitHub repository.

## 5. Test certificates

For testing without Apple Developer certificates, generate self-signed certificates using:

```bash
./create-test-certs.sh
```

These certificates are intended for CI testing only. Official macOS releases require valid Apple Developer
