#!/usr/bin/env bash

set -euo pipefail

workspace_root=${1:?workspace root is required}
sysroot="${workspace_root}/.debug/zynq-sysroot"

mkdir -p "$sysroot"

# Clean files that were previously pulled by an overbroad */debug/* matcher.
rm -rf "$sysroot/usr/lib/node_modules"

downloaded=0
skipped=0

while IFS= read -r remote_path; do
    [[ -n "$remote_path" ]] || continue

    local_path="${sysroot}${remote_path}"
    if [[ -e "$local_path" ]]; then
        skipped=$((skipped + 1))
        continue
    fi

    mkdir -p "$(dirname "$local_path")"
    scp -p "zynq:${remote_path}" "$local_path"
    downloaded=$((downloaded + 1))
done < <(
    ssh zynq '
        for dir in /lib /usr/lib /usr/local/lib; do
            [ -d "$dir" ] || continue
            find "$dir" \( -type f -o -type l \) \( \
                -path "*/.debug/*" -o \
                -name "*.so" -o \
                -name "*.so.*" -o \
                -name "*.debug" -o \
                -name "ld-linux*.so*" \
            \)
        done | sort -u
    '
)

printf 'Zynq debug sysroot ready: downloaded %d, skipped %d existing files.\n' "$downloaded" "$skipped"