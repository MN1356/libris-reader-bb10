#!/usr/bin/env bash
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BAR_FILE="$SCRIPT_DIR/build/device/Libris-unsigned.bar"
DEVICE_IP="${1:-169.254.0.1}"

CONTAINER_CLI="podman"
if ! command -v podman &> /dev/null; then
    if command -v docker &> /dev/null; then
        CONTAINER_CLI="docker"
    else
        echo "Lỗi: Không tìm thấy podman hoặc docker trên hệ điều hành host!" >&2
        exit 1
    fi
fi

if [[ ! -f "$BAR_FILE" ]]; then
    echo "Thông báo: Không tìm thấy $BAR_FILE! Đang tiến hành build..."
    "$SCRIPT_DIR/build.sh" device
fi

if [[ -z "$BB10_PASSWORD" ]]; then
    read -rsp "Nhập mật khẩu Development Mode của BlackBerry 10 ($DEVICE_IP): " BB10_PASSWORD
    echo ""
fi

echo "Đang kiểm tra và đóng tiến trình cũ nếu đang chạy trên $DEVICE_IP..."
$CONTAINER_CLI run --rm --net=host \
    -v "$SCRIPT_DIR/build/device:/output:ro,z" \
    docker.io/accupara/bbndk:latest \
    blackberry-deploy -terminateApp \
        -device "$DEVICE_IP" \
        -password "$BB10_PASSWORD" \
        -package /output/Libris-unsigned.bar 2>/dev/null || true

echo "Đang triển khai Libris vào thiết bị $DEVICE_IP sử dụng $CONTAINER_CLI..."
$CONTAINER_CLI run --rm --net=host \
    -v "$SCRIPT_DIR/build/device:/output:ro,z" \
    docker.io/accupara/bbndk:latest \
    blackberry-deploy -installApp -launchApp \
        -device "$DEVICE_IP" \
        -password "$BB10_PASSWORD" \
        /output/Libris-unsigned.bar

echo "Hoàn tất! Libris đã được cài đặt và kích hoạt trên máy."
