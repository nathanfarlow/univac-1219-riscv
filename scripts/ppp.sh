#!/usr/bin/env bash
set -e

PORT="/dev/cu.usbmodem123451"
BAUD=9600
TAP="/tmp/ppp-tap"
LOCAL_IP="10.0.0.1"
REMOTE_IP="10.0.0.2"

cleanup() {
    echo -e "\nShutting down..."
    [[ -n "$PPPD_PID" ]] && kill "$PPPD_PID" 2>/dev/null
    [[ -n "$SOCAT_PID" ]] && kill "$SOCAT_PID" 2>/dev/null
    wait 2>/dev/null
    echo "Done."
}
trap cleanup EXIT INT TERM

echo "Starting socat tap on $PORT..."
sudo socat -x -v \
     PTY,link="$TAP",raw,echo=0,mode=666 \
     "$PORT",raw,echo=0,ispeed="$BAUD",ospeed="$BAUD" &
SOCAT_PID=$!
sleep 1

if [[ ! -e "$TAP" ]]; then
    echo "Error: socat failed to create $TAP"
    exit 1
fi

echo "Starting pppd on $TAP..."
sudo pppd "$TAP" "$BAUD" \
     noauth local nodetach debug \
     "$LOCAL_IP":"$REMOTE_IP" \
     nodefaultroute noccp novj noaccomp nopcomp noipv6 &
PPPD_PID=$!

echo "Running. Ctrl+C to stop."
wait
