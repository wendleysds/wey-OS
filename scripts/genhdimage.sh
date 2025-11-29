#!/usr/bin/env bash

# genhdimage.sh
# Usage: genhdimage.sh [BZIMAGE] [HDIMAGE]
# If BZIMAGE is not provided this will try to find a file named "bzImage" under the current tree.
# Default HDIMAGE is ./build/hdimage.img (32 MiB)

BZIMAGE="${1:-}"
HDIMAGE="${2:-./build/hdimage.img}"
TMPBOOT="$(mktemp --tmpdir boot.cfg.XXXXXX)"

cleanup() {
	rm -f "$TMPBOOT"
}
trap cleanup EXIT

FAT_PY="tools/fs/fat/main.py"
BOOR_PY="tools/boot/embed/install"

# locate bzImage if not given
if [[ -z "$BZIMAGE" ]]; then
	BZIMAGE="$(find . -type f -name 'bzImage' -print -quit || true)"
	if [[ -z "$BZIMAGE" ]]; then
		echo "error: bzImage not provided and none found in current tree." >&2
		exit 1
	fi
fi

if [[ ! -f "$BZIMAGE" ]]; then
	echo "error: bzImage '$BZIMAGE' not found." >&2
	exit 1
fi

echo "Using bzImage: $BZIMAGE"
echo "Creating HD image: $HDIMAGE (32 MiB)"

# create zero-filled image, 32 MiB
dd if=/dev/zero of="$HDIMAGE" bs=1M count=32 status=none

# install bootloader
python3 "$BOOR_PY" "$HDIMAGE"

# create temporary boot.cfg content
echo "TARGET=/boot/kernel" > "$TMPBOOT"

# copy files into image using fat.py
echo "Copying kernel into image..."
python3 "$FAT_PY" "$HDIMAGE" cp -ex "$BZIMAGE" /boot/kernel

echo "Copying boot.cfg into image..."
python3 "$FAT_PY" "$HDIMAGE" cp -ex "$TMPBOOT" /boot/boot.cfg

echo "Done. Image: $HDIMAGE"
