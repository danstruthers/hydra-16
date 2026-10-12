#!/bin/sh
# shot.sh FILE.svg|FILE.html OUT.png [WIDTH] [HEIGHT] - a picture of a drawing or page, for checking (Edge, a fresh profile)
F=$(cd "$(dirname "$1")" && pwd)/$(basename "$1"); O=$(cd "$(dirname "$2")" && pwd)/$(basename "$2"); W=${3:-1800}; H=${4:-1500}
P=$(mktemp -d)
case "$F" in *.svg) printf '<html><body style="margin:0;background:#fff"><img src="%s" style="width:%spx"></body></html>' "$(basename "$F")" "$W" > "$(dirname "$F")/_shot.html"; U="$(dirname "$F")/_shot.html";; *) U=$F;; esac
"/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe" --headless --disable-gpu --user-data-dir="$(cygpath -w "$P")" --screenshot="$(cygpath -w "$O")" --window-size=$W,$H "file:///$(cygpath -w "$U")" 2>/dev/null
rm -rf "$P" "$(dirname "$F")/_shot.html"
