#!/usr/bin/env bash

set -euo pipefail

workspace_root=${1:?workspace root is required}
local_root="${workspace_root}/zynq_files"
remote_root="/home/em-os"

declare -A remote_meta
declare -A directories_to_create
files_to_copy=()
copied=0
skipped=0

while IFS=$'\t' read -r rel_path size mtime; do
    [[ -n "$rel_path" ]] || continue
    remote_meta["$rel_path"]="${size}:${mtime%%.*}"
done < <(
    ssh zynq "if [ -d '$remote_root' ]; then cd '$remote_root' && find . -type f -printf '%P\t%s\t%T@\n'; fi"
)

while IFS=$'\t' read -r rel_path size mtime; do
    [[ -n "$rel_path" ]] || continue

    local_meta="${size}:${mtime%%.*}"
    if [[ ${remote_meta["$rel_path"]-} == "$local_meta" ]]; then
        skipped=$((skipped + 1))
        continue
    fi

    files_to_copy+=("$rel_path")
    directories_to_create["$(dirname "$rel_path")"]=1
done < <(
    cd "$local_root" && find . -type f -printf '%P\t%s\t%T@\n' | sort
)

if [[ ${#files_to_copy[@]} -gt 0 ]]; then
    {
        printf 'set -e\n'
        for dir_path in "${!directories_to_create[@]}"; do
            if [[ "$dir_path" == "." ]]; then
                printf 'mkdir -p %q\n' "$remote_root"
            else
                printf 'mkdir -p %q\n' "$remote_root/$dir_path"
            fi
        done
    } | ssh zynq sh
fi

for rel_path in "${files_to_copy[@]}"; do
    remote_path="$remote_root/$rel_path"
    remote_temp_path="$remote_path.copilot-tmp.$$"
    scp -p "$local_root/$rel_path" "zynq:$remote_temp_path"
    ssh zynq "mv '$remote_temp_path' '$remote_path'"
    copied=$((copied + 1))
done

printf 'Zynq file sync ready: copied %d, skipped %d existing files.\n' "$copied" "$skipped"
