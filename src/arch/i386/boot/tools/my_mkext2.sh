#!/bin/bash

set -ex

# Arguments:
# FS_DD_BS
# FS_DD_COUNT
# SOURCE_SYSROOT
# SOURCE_SYSSKEL
# CHRONOS_OBJ
# USER_BIN
# TARGET
# USER

echo "Super user privileges are needed for loop mounting..."
sudo echo ""

dd if=/dev/zero of="$TARGET" bs=$FS_DD_BS count=$FS_DD_COUNT seek=0
echo "yes" | /sbin/mkfs.ext2 $TARGET
TEMPDIR="$(mktemp -d)"
sudo mount -o loop "$TARGET" "$TEMPDIR"

sudo chown -R $USER:$USER "$TEMPDIR"
cp -R "$SOURCE_SYSROOT/"* "$TEMPDIR"


function gensysskel () {
	find "$SOURCE_SYSSKEL" -type d | while read DIR; do
		mkdir -p "$TEMPDIR/$DIR"
	done
}

gensysskel

cp -R $SOURCE_SYSSKEL/* "$TEMPDIR"
mkdir -p "$TEMPDIR/bin"
cp -R $USER_BIN/* "$TEMPDIR/bin/"
mkdir -p "$TEMPDIR/boot"
cp "$CHRONOS_OBJ" "$TEMPDIR/boot/chronos.elf"

sync "$TEMPDIR"
sudo umount -l "$TEMPDIR"
#rm -rf "$TEMPDIR"
