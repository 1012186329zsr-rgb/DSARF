#!/usr/bin/env bash
set -euo pipefail

# Create a clean source release archive from the current repository
# while excluding generated/local artifacts.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

STAMP="$(date +%Y%m%d_%H%M%S)"
ARCHIVE_NAME="topology-master-vc3_release_${STAMP}.tar.gz"
ARCHIVE_PATH="${REPO_ROOT}/${ARCHIVE_NAME}"

cd "${REPO_ROOT}"

tar \
  --exclude-vcs \
  --exclude='./build' \
  --exclude='./bin' \
  --exclude='./obj' \
  --exclude='./temp' \
  --exclude='./.vscode' \
  --exclude='./*.tar.gz' \
  --exclude='./*.zip' \
  -czf "${ARCHIVE_PATH}" \
  .

echo "Created: ${ARCHIVE_PATH}"
