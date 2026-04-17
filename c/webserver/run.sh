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

echo "Starting socat..." >&2
sudo socat pty,link=$PTY1,raw,echo=0,mode=666 pty,link=$PTY2,raw,echo=0,mode=666 &
sleep 0.5
ls -la $PTY1 $PTY2 >&2

echo "Starting pppd..." >&2
sudo pppd $PTY1 115200 noauth nodetach local debug \
    10.0.0.1:10.0.0.2 nodefaultroute noccp novj noaccomp nopcomp noipv6 &
sleep 0.5

echo "=== Ready! curl http://10.0.0.2/ ===" >&2

make -s "$MODE" <> $PTY2 >&0
