#!/usr/bin/env bash

set -euo pipefail

ZYNQ_SUDO_PASSWORD="${ZYNQ_SUDO_PASSWORD:-0}"

ssh -o LogLevel=ERROR zynq "printf '%s\\n' '${ZYNQ_SUDO_PASSWORD}' | sudo -S -p '' sh -c 'pkill -9 -x gdbserver || true; pkill -9 -x controller_core || true; pkill -9 -x controller_editor_service || true; pkill -9 -x controller_edito || true; pkill -9 -x controller_edit || true; rm -f /tmp/controller_editor_service.pid || true'"
