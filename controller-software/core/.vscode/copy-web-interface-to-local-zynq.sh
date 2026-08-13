#!/usr/bin/env bash

set -euo pipefail

workspace_root=${1:?workspace root is required}
web_root="${workspace_root}/../web-interface"
client_target="${workspace_root}/zynq_files/controller/web/client"
legacy_client_root="${web_root}/client"
local_node_bin="${web_root}/.tools/node/bin"

build_ok=0
npm_cmd=""

if [[ -x "${local_node_bin}/node" && -x "${local_node_bin}/npm" ]]; then
    npm_cmd="${local_node_bin}/npm"
elif [[ -x "/usr/bin/node" && -x "/usr/bin/npm" ]]; then
    npm_cmd="/usr/bin/npm"
fi

if [[ -n "$npm_cmd" ]]; then
    cd "$web_root"
    export PATH="${local_node_bin}:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

    if [[ ! -x "$web_root/node_modules/.bin/vite" ]]; then
        echo "[info] Installing web-interface dependencies"
        "$npm_cmd" install
    fi

    if "$npm_cmd" run build; then
        build_ok=1
    else
        echo "[warn] web-interface build failed; falling back to legacy static client sync" >&2
    fi
else
    echo "[warn] Linux npm/node not found; falling back to legacy static client sync" >&2
fi

mkdir -p "$client_target"

if [[ $build_ok -eq 1 && -d "$web_root/dist" ]]; then
    rsync -av --delete "$web_root/dist/" "$client_target/"
    rm -f "$client_target/editor.js" "$client_target/editor.css"
    rm -rf "$client_target/login" "$client_target/spa_1"
    echo "Svelte dist synced to $client_target"
else
    rsync -av --delete "$legacy_client_root/" "$client_target/"
    echo "Legacy client synced to $client_target"
fi
