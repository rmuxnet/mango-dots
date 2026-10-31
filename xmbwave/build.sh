#!/bin/sh
set -eu
cd "$(dirname "$0")"

mkdir -p build

WLR_PROTO=/usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml
XDG_PROTO=/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

for p in "$WLR_PROTO" "$XDG_PROTO"; do
  [ -f "$p" ] || { echo "missing $p"; exit 1; }
done

wayland-scanner client-header "$WLR_PROTO" build/wlr-layer-shell-unstable-v1-client-protocol.h
wayland-scanner private-code  "$WLR_PROTO" build/wlr-layer-shell-unstable-v1-protocol.c
wayland-scanner client-header "$XDG_PROTO" build/xdg-shell-client-protocol.h
wayland-scanner private-code  "$XDG_PROTO" build/xdg-shell-protocol.c

for s in bg wave particles; do
  for k in vert frag; do
    python3 - <<PY > "build/${s}.${k}.inc"
import sys
src = open("shaders/${s}.${k}").read()
out = []
for line in src.splitlines():
    out.append('"' + line.replace('\\\\','\\\\\\\\').replace('"','\\\\"') + '\\\\n"')
print("\\n".join(out))
PY
  done
done

CXXFLAGS="-std=c++20 -O2 -pipe -Wall -Wno-unused-parameter -fno-exceptions -fno-rtti"
CFLAGS="-O2 -pipe -w"
LIBS="$(pkg-config --libs wayland-client wayland-egl egl glesv2) -lm"

clang $CFLAGS -c build/wlr-layer-shell-unstable-v1-protocol.c -o build/wlr-layer-shell-unstable-v1-protocol.o
clang $CFLAGS -c build/xdg-shell-protocol.c                   -o build/xdg-shell-protocol.o

clang++ $CXXFLAGS -Ibuild -Isrc \
  src/main.cpp src/spline.cpp \
  build/wlr-layer-shell-unstable-v1-protocol.o \
  build/xdg-shell-protocol.o \
  $LIBS \
  -o build/xmbwave-sway

echo "built build/xmbwave-sway"
