#!/usr/bin/env bash
set -euo pipefail

# zip2ship.sh
# Create a clean source ZIP of the Flowcore/Flowmini development tree.
#
# The script is location-independent: it archives the repository it belongs to,
# not the caller's current working directory.

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Intended location:
#   <repo>/tools/zip2ship.sh
#
# If placed at repository root instead, fall back cleanly.
if [[ -d "${script_dir}/../.git" ]]; then
    repo="$(cd -- "${script_dir}/.." && pwd)"
elif [[ -d "${script_dir}/.git" ]]; then
    repo="${script_dir}"
else
    echo "error: cannot locate repository root from: ${script_dir}" >&2
    exit 2
fi

project_name="$(basename -- "${repo}")"
archive="${HOME}/${project_name}-current.zip"

echo "repo:    ${repo}"
echo "archive: ${archive}"

rm -f -- "${archive}"

cd -- "${repo}"

zip -r "${archive}" . \
    -x '.git/*' \
       'cmake-build-*/*' \
       '*/cmake-build-*/*' \
       'build/*' \
       '*/build/*' \
       '.idea/*' \
       '*/.idea/*' \
       '*.zip' \
       '*.o' \
       '*.a' \
       '*.so' \
       '*.so.*'

echo
echo "created: ${archive}"
