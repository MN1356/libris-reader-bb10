#!/usr/bin/env bash
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="MN1356/libris-reader-bb10"
BUMP_MODE=""
REBUILD=0
CUSTOM_TAG=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --major) BUMP_MODE="major"; REBUILD=1; shift ;;
        --minor) BUMP_MODE="minor"; REBUILD=1; shift ;;
        --patch) BUMP_MODE="patch"; REBUILD=1; shift ;;
        --rebuild) REBUILD=1; shift ;;
        --tag) CUSTOM_TAG="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: ./release.sh [--major|--minor|--patch] [--rebuild] [--tag vX.Y.Z]"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

cd "$SCRIPT_DIR"

if [[ -n "$BUMP_MODE" ]]; then
    echo "--> Building new $BUMP_MODE release..."
    BUMP_TYPE="$BUMP_MODE" ./build.sh device all
elif [[ "$REBUILD" == "1" || ! -f "build/device/Libris-unsigned.bar" ]]; then
    echo "--> Building device BAR (preserving current buildId)..."
    NO_BUMP=1 ./build.sh device all
fi

VERSION="$(sed -n 's/^#define LIBRIS_APP_VERSION "\(.*\)"/\1/p' src/BuildInfo.hpp | head -n1)"
BUILD="$(sed -n 's/^#define LIBRIS_BUILD_NUMBER "\(.*\)"/\1/p' src/BuildInfo.hpp | head -n1)"
TAG="${CUSTOM_TAG:-v${VERSION}}"
ASSET_PATH="build/device/Libris-v${VERSION}-b${BUILD}.bar"
NOTES_PATH="build/device/RELEASE_NOTES.md"

cp "build/device/Libris-unsigned.bar" "$ASSET_PATH"

python3 - "$TAG" "$BUILD" "$NOTES_PATH" <<'PY'
import os, sys
tag, bld, out_path = sys.argv[1], sys.argv[2], sys.argv[3]

def top_section(path):
    if not os.path.exists(path):
        return ""
    out = []
    started = False
    for line in open(path, encoding="utf-8").read().splitlines():
        if not started:
            if line.strip():
                started = True
                out.append(line)
        else:
            if not line.strip():
                break
            out.append(line)
    return "\n".join(out)

en_notes = top_section("assets/changelog_en.txt")
vi_notes = top_section("assets/changelog_vi.txt")
with open(out_path, "w", encoding="utf-8") as f:
    f.write(f"## Libris {tag} (Build {bld})\n\n")
    if en_notes:
        f.write("### English\n" + en_notes + "\n\n")
    if vi_notes:
        f.write("### Tiếng Việt\n" + vi_notes + "\n")
PY

echo "--> Prepared asset: $ASSET_PATH"
echo "--> Prepared notes: $NOTES_PATH"

if command -v gh &> /dev/null && gh auth status &> /dev/null; then
    echo "--> Creating GitHub Release $TAG on $REPO..."
    if gh release view "$TAG" --repo "$REPO" &> /dev/null; then
        gh release upload "$TAG" "$ASSET_PATH" --repo "$REPO" --clobber
        gh release edit "$TAG" --repo "$REPO" --title "Libris $TAG (Build $BUILD)" --notes-file "$NOTES_PATH"
    else
        gh release create "$TAG" "$ASSET_PATH" \
            --repo "$REPO" \
            --title "Libris $TAG (Build $BUILD)" \
            --notes-file "$NOTES_PATH"
    fi
    echo "=== GitHub Release published: https://github.com/$REPO/releases/tag/$TAG ==="
else
    if ! git rev-parse -q --verify "refs/tags/$TAG" >/dev/null; then
        git tag -a "$TAG" -m "Libris $TAG (Build $BUILD)"
        echo "--> Created local git tag: $TAG"
    else
        echo "--> Git tag $TAG already exists locally."
    fi
    echo ""
    echo "GitHub CLI (gh) chưa cài hoặc chưa đăng nhập trên máy này."
    echo "Để phát hành tự động qua GitHub Actions (.github/workflows/release.yml), hãy chạy:"
    echo "  git push origin master && git push origin $TAG"
    echo "Hoặc tải thủ công file sau lên mục GitHub Releases:"
    echo "  $ASSET_PATH"
fi
