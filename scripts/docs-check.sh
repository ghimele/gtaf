#!/usr/bin/env bash
set -euo pipefail

BASE_REF="${1:-origin/main}"

echo "Running documentation checks against $BASE_REF"
echo

# CHANGED_FILES=$(git diff --name-only "$BASE_REF")
CHANGED_FILES=$(find docs -type f)

# ---------------------------------------------
# 1. Required headers for non-ADR docs
# ---------------------------------------------
echo "== Checking required headers =="

for file in $(echo "$CHANGED_FILES" | grep '^docs/' | grep -v '^docs/adr/' | grep '\.md$' || true); do
  if [[ "$file" == *"README.md"* ]] || [[ "$file" == *"/_template/"* ]]; then
    continue
  fi

  echo "Checking headers in $file"

  head -n 20 "$file" | grep -q "\*\*Status:\*\*" \
    || { echo "ERROR: Missing Status in $file"; exit 1; }

  head -n 20 "$file" | grep -q "\*\*Last updated:\*\*" \
    || { echo "ERROR: Missing Last updated in $file"; exit 1; }

  head -n 20 "$file" | grep -q "\*\*Owner:\*\*" \
    || { echo "ERROR: Missing Owner in $file"; exit 1; }

  head -n 20 "$file" | grep -q "\*\*Related ADRs:\*\*" \
    || { echo "ERROR: Missing Related ADRs in $file"; exit 1; }
done

echo "Header checks passed."
echo

# ---------------------------------------------
# 2. ADR immutability
# ---------------------------------------------
echo "== Enforcing ADR immutability =="

for adr in $(echo "$CHANGED_FILES" | grep '^docs/adr/ADR-' || true); do
  if grep -q "\*\*Status:\*\* Accepted" "$adr"; then
    echo "ERROR: Accepted ADR modified: $adr"
    exit 1
  fi
done

echo "ADR immutability checks passed."
echo

# ---------------------------------------------
# 3. File naming conventions
# ---------------------------------------------
echo "== Checking file naming conventions =="

BAD_FILES=$(echo "$CHANGED_FILES" \
  | grep '^docs/' \
  | grep -v '^docs/adr/' \
  | grep -v '/README\.md$' \
  | grep -v '/STYLE\.md$' \
  | grep -E '[A-Z ]' \
  || true)

if [[ -n "$BAD_FILES" ]]; then
  echo "ERROR: Invalid file names detected:"
  echo "$BAD_FILES"
  exit 1
fi

echo "File naming checks passed."
echo

# ---------------------------------------------
# 4. Decision language outside ADRs
# ---------------------------------------------
echo "== Checking for decision language outside ADRs =="

for file in $(echo "$CHANGED_FILES" | grep '^docs/' | grep -v '^docs/adr/' | grep '\.md$' || true); do
  if grep -Ei "we decided|we chose|the decision was" "$file"; then
    echo "ERROR: Decision language found in $file"
    exit 1
  fi
done

echo "Decision language checks passed."
echo

# ---------------------------------------------
# 5. Markdown lint
# ---------------------------------------------
echo "== Running markdownlint =="

npx markdownlint-cli2 "**/*.md" "#.github/"

echo
echo "All documentation checks passed."
