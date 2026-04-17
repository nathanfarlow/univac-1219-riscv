#!/bin/bash
# Run webserver with pppd
set -e
cd "$(dirname "$0")"

MODE="${1:-ocaml-run}"
PTY1=/tmp/webserver-pty1
PTY2=/tmp/webserver-pty2

cleanup() {
    sudo killall pppd 2>/dev/null || true
    sudo pkill -f "socat.*webserver" 2>/dev/null || true
    sudo rm -f $PTY1 $PTY2
}
trap cleanup EXIT
cleanup; sleep 0.5

sudo socat pty,link=$PTY1,raw,echo=0,mode=666 pty,link=$PTY2,raw,echo=0,mode=666 &
sleep 0.5
sudo pppd $PTY1 115200 noauth nodetach local \
    10.0.0.1:10.0.0.2 nodefaultroute noccp novj noaccomp nopcomp noipv6 &
sleep 0.5

echo "=== Ready! curl http://10.0.0.2/ ==="

make -s "$MODE" <> $PTY2 >&0
