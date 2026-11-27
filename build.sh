#!/usr/bin/env bash
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="${1:-device}" # "device" (armle-v7) or "simulator" (x86)
ACTION="${2:-all}"    # "build", "package", or "all"

IMAGE="docker.io/accupara/bbndk:latest"
CONTAINER_CLI="podman"
if ! command -v podman &> /dev/null; then
    if command -v docker &> /dev/null; then
        CONTAINER_CLI="docker"
    else
        echo "Error: Neither podman nor docker found on host!" >&2
        exit 1
    fi
fi

if [[ "$TARGET" == "simulator" ]]; then
    ARCH="x86"
    SPEC="blackberry-x86-qcc"
    CONFIG="simulator"
    VARIANT="o-g"
    BAR_CONFIG="Simulator-Debug"
    BAR_NAME="Libris-simulator-unsigned.bar"
else
    ARCH="armle-v7"
    SPEC="blackberry-armv7le-qcc"
    CONFIG="device"
    VARIANT="o.le-v7-g"
    BAR_CONFIG="Device-Debug"
    BAR_NAME="Libris-unsigned.bar"
fi

BUILD_DIR="$SCRIPT_DIR/build/$TARGET"
mkdir -p "$BUILD_DIR"

if [[ "$ACTION" == "package" || "$ACTION" == "all" ]]; then
    if [[ "${AUTO_BUMP:-1}" != "0" && "${NO_BUMP:-0}" != "1" ]]; then
        CURRENT_BUILD=$(grep -oE "<buildId>[0-9]+</buildId>" "$SCRIPT_DIR/bar-descriptor.xml" | grep -oE "[0-9]+" || echo "1")
        NEXT_BUILD=$((CURRENT_BUILD + 1))
        sed -i -E "s|<buildId>[0-9]+</buildId>|<buildId>$NEXT_BUILD</buildId>|" "$SCRIPT_DIR/bar-descriptor.xml"
        echo "--> Auto-incremented buildId: $CURRENT_BUILD -> $NEXT_BUILD"
        python3 "$SCRIPT_DIR/scripts/update_build_metadata.py" --project-dir "$SCRIPT_DIR" --bumped --bump-version "${BUMP_TYPE:-patch}"
    else
        CURRENT_BUILD=$(grep -oE "<buildId>[0-9]+</buildId>" "$SCRIPT_DIR/bar-descriptor.xml" | grep -oE "[0-9]+" || echo "1")
        echo "--> Preserved buildId: $CURRENT_BUILD (auto-bump disabled)"
        python3 "$SCRIPT_DIR/scripts/update_build_metadata.py" --project-dir "$SCRIPT_DIR"
    fi
else
    python3 "$SCRIPT_DIR/scripts/update_build_metadata.py" --project-dir "$SCRIPT_DIR"
fi

echo "=== Building Libris for $TARGET ($ARCH) using $CONTAINER_CLI ==="

USERNS_FLAGS=()
if [[ "$CONTAINER_CLI" == "podman" ]]; then
    USERNS_FLAGS=(--userns=keep-id)
else
    USERNS_FLAGS=(--user "$(id -u):$(id -g)")
fi

$CONTAINER_CLI run --rm --platform=linux/amd64 \
    "${USERNS_FLAGS[@]}" \
    --tmpfs /tmp:rw,exec,size=1024m,mode=1777 \
    -v "$SCRIPT_DIR:/project:ro,z" \
    -v "$BUILD_DIR:/output:rw,z" \
    --entrypoint /bin/bash "$IMAGE" -c "
        set -eo pipefail
        export QNX_CONFIGURATION=/tmp/qnx
        mkdir -p \"\$QNX_CONFIGURATION\"
        export MAKEFLAGS=\"-I\$QNX_TARGET/usr/include\"
        export CPUVARDIR=\"$ARCH\"

        mkdir -p /tmp/workspace
        cp -R /project/src /tmp/workspace/
        cp -R /project/assets /tmp/workspace/
        cp /project/Libris.pro /tmp/workspace/
        cp /project/bar-descriptor.xml /tmp/workspace/
        cp /project/icon-*.png /project/splash-*.png /tmp/workspace/ 2>/dev/null || true
        cd /tmp/workspace

        if [[ \"$ACTION\" == \"build\" || \"$ACTION\" == \"all\" ]]; then
            echo '--> Running qmake...'
            qmake -spec \"$SPEC\" Libris.pro \"CONFIG+=$CONFIG\" CONFIG+=debug CONFIG-=release
            echo '--> Compiling C++ and Cascades code...'
            make -j\$(nproc || echo 4)
            cp \"$VARIANT/Libris\" /output/Libris
            echo '--> Binary created: /output/Libris'
        fi

        if [[ \"$ACTION\" == \"package\" || \"$ACTION\" == \"all\" ]]; then
            echo '--> Packaging BAR application...'
            cp /output/Libris /tmp/workspace/
            blackberry-nativepackager -package \"$BAR_NAME\" \
                -devMode -platformVersion 10.2.0.0 \
                -configuration \"$BAR_CONFIG\" bar-descriptor.xml
            cp \"$BAR_NAME\" /output/
            echo '--> Package created: /output/$BAR_NAME'
        fi
        echo '=== Build successful! ==='
    "

echo "Artifacts are ready in $BUILD_DIR:"
ls -lh "$BUILD_DIR"
