#!/usr/bin/env bash

# genhdimage.sh
#
# Usage:
#   genhdimage.sh [BZIMAGE] [HDIMAGE]
#
# If BZIMAGE is not provided, searches for a file named "bzImage"
# under the current directory.
#
# Default HDIMAGE:
#   ./build/hdimage
#
# Creates a 1.44 MiB floppy disk image.

set -Eeuo pipefail

readonly SCRIPT_NAME="$(basename "$0")"
readonly NOVALOADER_DIR="tmp/NovaLoader"
readonly TMPBOOT="$(mktemp --tmpdir boot.cfg.XXXXXX)"

BZIMAGE="${1:-}"
HDIMAGE="${2:-./build/hdimage}"

cleanup() {
    rm -f "$TMPBOOT"
}

die() {
    echo "error: $*" >&2
    exit 1
}

trap cleanup EXIT

# Helpers

ensure_relative_path() {
    local path="$1"

    case "$path" in
        ""|/*|./*|../*)
            printf '%s\n' "$path"
            ;;
        *)
            printf './%s\n' "$path"
            ;;
    esac
}

# Locate bzImage
if [[ -z "$BZIMAGE" ]]; then
    echo "Searching for bzImage..."

    BZIMAGE="$(
        find . \
            -type f \
            -name 'bzImage' \
            -print \
            -quit
    )"

    [[ -n "$BZIMAGE" ]] ||
        die "bzImage not provided and none found in current tree."
fi

BZIMAGE="$(ensure_relative_path "$BZIMAGE")"
HDIMAGE="$(ensure_relative_path "$HDIMAGE")"

[[ -f "$BZIMAGE" ]] ||
    die "bzImage '$BZIMAGE' not found."

# Prepare output
mkdir -p "$(dirname "$HDIMAGE")"

echo "Using bzImage: $BZIMAGE"
echo "Creating HD image: $HDIMAGE (1.44 MiB)"

# 1.44 MiB floppy = 2880 sectors × 512 bytes.
dd \
    if=/dev/zero \
    of="$HDIMAGE" \
    bs=512 \
    count=2880 \
    status=none

# Prepare NovaLoader

mkdir -p tmp

if [[ ! -d "$NOVALOADER_DIR" ]]; then
    read -r -p \
        "NovaLoader not found. Clone from GitHub? (y/n): " \
        REPLY

    if [[ "$REPLY" =~ ^[Yy]$ ]]; then
        git clone \
            --depth 1 \
            https://github.com/wendleysds/NovaLoader.git \
            "$NOVALOADER_DIR"
    else
        die "NovaLoader not found."
    fi
fi

[[ -d "$NOVALOADER_DIR" ]] ||
    die "NovaLoader directory '$NOVALOADER_DIR' does not exist."

echo "Building NovaLoader..."

make -C "$NOVALOADER_DIR"

echo "Installing bootloader..."

python3 \
    "$NOVALOADER_DIR/install" \
    -p 1 \
    -f 1 \
    2880 \
    "$HDIMAGE" \
    -Y

# Generate boot.cfg
cat > "$TMPBOOT" <<'EOF'
set timeout 5
entry WeyOs {
load /boot/kernel
boot
}
menu
EOF

echo "Installing boot.cfg..."

python3 \
    "$NOVALOADER_DIR/tools/fat.py" \
    put \
    "$HDIMAGE" \
    "$TMPBOOT" \
    /boot/boot.cfg

echo "Installing kernel..."

python3 \
    "$NOVALOADER_DIR/tools/fat.py" \
    put \
    "$HDIMAGE" \
    "$BZIMAGE" \
    /boot/kernel

echo
echo "Done. Image: $HDIMAGE"