#!/usr/bin/env bash
set -euo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

for f in "$dir"/CFML_FUNCTION_*.md; do
    name="${f##*/}"
    name="${name#CFML_FUNCTION_}"
    name="${name%.md}"
    pct="$(grep '^## Implemented:' "$f" | head -1 | sed 's/^## Implemented:[[:space:]]*//; s/[[:space:]]*$//')"
    dep="$(awk '/^## Dependency/{getline; while ($0=="") getline; print; exit}' "$f")"
    printf '%-30s %-5s %s\n' "$name" "${pct:-N/A}" "${dep:-N/A}"
done
