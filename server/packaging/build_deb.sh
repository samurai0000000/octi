#!/bin/bash
#
# build_deb.sh
#
# Copyright (C) 2026, Charles Chiou
#

set -e

if [ $# -lt 3 ]; then
    echo "Usage: $0 <binary_path> <version> <output_dir> [arch]" >&2
    exit 1
fi

BINARY_PATH="$1"
VERSION="$2"
OUTPUT_DIR="$3"
ARCH="${4:-$(dpkg --print-architecture 2>/dev/null || echo "amd64")}"

if [ ! -f "$BINARY_PATH" ]; then
    echo "Error: binary not found: $BINARY_PATH" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(cd "$OUTPUT_DIR" && pwd)"

STAGING_DIR="${OUTPUT_DIR}/deb-staging/octi-server_${VERSION}_${ARCH}"
rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR/DEBIAN"
mkdir -p "$STAGING_DIR/usr/sbin"
mkdir -p "$STAGING_DIR/lib/systemd/system"
mkdir -p "$STAGING_DIR/etc/default"
mkdir -p "$STAGING_DIR/etc/octi"

cp "$BINARY_PATH" "$STAGING_DIR/usr/sbin/octi-server"
chmod 0755 "$STAGING_DIR/usr/sbin/octi-server"
if command -v strip >/dev/null 2>&1; then
    strip --strip-unneeded "$STAGING_DIR/usr/sbin/octi-server" || true
fi

cp "$SCRIPT_DIR/octi-server.service" "$STAGING_DIR/lib/systemd/system/octi-server.service"
chmod 0644 "$STAGING_DIR/lib/systemd/system/octi-server.service"

cp "$SCRIPT_DIR/octi-server.default" "$STAGING_DIR/etc/default/octi-server"
chmod 0644 "$STAGING_DIR/etc/default/octi-server"

cp "$SCRIPT_DIR/oidc.json" "$STAGING_DIR/etc/octi/oidc.json"
chmod 0600 "$STAGING_DIR/etc/octi/oidc.json"
chmod 0750 "$STAGING_DIR/etc/octi"

cp "$SCRIPT_DIR/conffiles" "$STAGING_DIR/DEBIAN/conffiles"
chmod 0644 "$STAGING_DIR/DEBIAN/conffiles"

cp "$SCRIPT_DIR/postinst" "$STAGING_DIR/DEBIAN/postinst"
chmod 0755 "$STAGING_DIR/DEBIAN/postinst"

cp "$SCRIPT_DIR/prerm" "$STAGING_DIR/DEBIAN/prerm"
chmod 0755 "$STAGING_DIR/DEBIAN/prerm"

cp "$SCRIPT_DIR/postrm" "$STAGING_DIR/DEBIAN/postrm"
chmod 0755 "$STAGING_DIR/DEBIAN/postrm"

SHLIBS=""
if command -v dpkg-shlibdeps >/dev/null 2>&1; then
    TMP_SHLIB="$(mktemp -d)"
    mkdir -p "$TMP_SHLIB/debian"
    cat << 'EOF' > "$TMP_SHLIB/debian/control"
Source: octi-server
Package: octi-server
Architecture: any
Description: dummy
EOF
    SHLIBS=$( (cd "$TMP_SHLIB" && dpkg-shlibdeps -O -e"${STAGING_DIR}/usr/sbin/octi-server" 2>/dev/null) | sed -n 's/^shlibs:Depends=//p' )
    rm -rf "$TMP_SHLIB"
fi

if [ -z "$SHLIBS" ]; then
    SHLIBS="libc6 (>= 2.38), libgcc-s1 (>= 3.0), libsqlite3-0 (>= 3.37.2), libssl3t64 (>= 3.0.0), libstdc++6 (>= 13.1)"
fi
DEPS="${SHLIBS}, adduser"

INSTALLED_SIZE=$(du -sk "$STAGING_DIR" | awk '{print $1}')

cat << EOF > "$STAGING_DIR/DEBIAN/control"
Package: octi-server
Version: ${VERSION}
Section: games
Priority: optional
Architecture: ${ARCH}
Installed-Size: ${INSTALLED_SIZE}
Depends: ${DEPS}
Maintainer: Charles Chiou <samurai@selfso.com>
Homepage: https://www.selfso.com/projects/octi/
Description: OCTI Multiplayer, SSO & Game Archive Server
 Authoritative online tournament, Elo rating, voice coordination,
 and replay archive server for the OCTI board game platform.
EOF
chmod 0644 "$STAGING_DIR/DEBIAN/control"

(cd "$STAGING_DIR" && find usr lib etc -type f | sort | while read -r f; do md5sum "$f"; done) > "$STAGING_DIR/DEBIAN/md5sums"
chmod 0644 "$STAGING_DIR/DEBIAN/md5sums"

DEB_FILE="${OUTPUT_DIR}/octi-server_${VERSION}_${ARCH}.deb"
dpkg-deb --root-owner-group --build "$STAGING_DIR" "$DEB_FILE"

echo "Built Debian package: $DEB_FILE"
