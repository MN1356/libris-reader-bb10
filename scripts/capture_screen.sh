#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT_FILE="${1:-$PROJECT_DIR/build/device/ui_capture.png}"
DEVICE_IP="${2:-169.254.0.1}"

CONTAINER_CLI="podman"
if ! command -v podman &> /dev/null; then
    CONTAINER_CLI="docker"
fi

if [[ -z "${BB10_PASSWORD:-}" ]]; then
    read -rsp "Password BB10 ($DEVICE_IP): " BB10_PASSWORD
    echo ""
fi

mkdir -p "$(dirname "$OUT_FILE")"
TMP_STAGE="$(mktemp -d)"
trap 'rm -rf "$TMP_STAGE"' EXIT
echo "capture" > "$TMP_STAGE/ui_capture.req"

USERNS_FLAGS=()
if [[ "$CONTAINER_CLI" == "podman" ]]; then
    USERNS_FLAGS=(--userns=keep-id)
fi

$CONTAINER_CLI run --rm --net=host "${USERNS_FLAGS[@]}" \
    -v "$TMP_STAGE:/stage:rw,z" \
    docker.io/accupara/bbndk:latest \
    bash -c "
        blackberry-deploy -putFile /stage/ui_capture.req data/ui_capture.req \
            -device '$DEVICE_IP' -password '$BB10_PASSWORD' \
            -package-name com.nhatlee.libris -package-id testDev_tlee_libris4c845dd3
        for i in 1 2 3 4 5; do
            sleep 0.7
            if blackberry-deploy -getFile data/ui_capture.png /stage/ui_capture.png \
                -device '$DEVICE_IP' -password '$BB10_PASSWORD' \
                -package-name com.nhatlee.libris -package-id testDev_tlee_libris4c845dd3 2>/dev/null; then
                exit 0
            fi
        done
        echo 'Screenshot file not created by running app yet.' >&2
        exit 1
    "

mv -f "$TMP_STAGE/ui_capture.png" "$OUT_FILE"
echo "Captured screenshot saved to: $OUT_FILE"
ls -lh "$OUT_FILE"
