#!/usr/bin/env bash
set -euo pipefail

# Run this script from any directory to vendor the latest stable yyjson release.
# Requires curl and Python 3. GitHub's releases/latest endpoint excludes draft
# and prerelease releases; use its tag rather than the upstream default branch.
# We keep only the two source files and their MIT license, without local edits.
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
temp_dir=$(mktemp -d "${TMPDIR:-/tmp}/csound-yyjson.XXXXXX")
trap 'rm -rf "$temp_dir"' EXIT

fetch() {
    curl --fail --location --silent --show-error --retry 3 "$1" -o "$2"
}

fetch https://api.github.com/repos/ibireme/yyjson/releases/latest \
    "$temp_dir/release.json"
tag=$(python3 -c 'import json, sys; print(json.load(sys.stdin)["tag_name"])' \
    < "$temp_dir/release.json")
encoded_tag=$(python3 -c 'import sys, urllib.parse; print(urllib.parse.quote(sys.argv[1], safe=""))' "$tag")
release_url="https://github.com/ibireme/yyjson/releases/tag/$encoded_tag"
raw_url="https://raw.githubusercontent.com/ibireme/yyjson/$encoded_tag"

# Download everything before touching the checked-in files. If a request fails,
# the existing copy stays intact and the EXIT trap removes the partial downloads.
fetch "$raw_url/src/yyjson.c" "$temp_dir/yyjson.c"
fetch "$raw_url/src/yyjson.h" "$temp_dir/yyjson.h"
fetch "$raw_url/LICENSE" "$temp_dir/LICENSE"
printf '# yyjson\n\nUnmodified source from [yyjson %s](%s).\n\nThe library uses the MIT license; see [LICENSE](LICENSE).\n' \
    "$tag" "$release_url" > "$temp_dir/README.md"

for file in yyjson.c yyjson.h LICENSE README.md; do
    cp "$temp_dir/$file" "$source_dir/$file"
done

# Before committing, review the upstream release notes and the source/license
# diff, then rebuild Csound and run its JSON tests (ctest --test-dir build -L
# json-udt --output-on-failure). Commit the source files, license and README
# together so the recorded release always matches the vendored code.
printf 'Updated yyjson to %s. Release notes: %s\n' "$tag" "$release_url"
