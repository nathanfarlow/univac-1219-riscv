import serial
import sys
import threading
import time

PORT = "/dev/cu.usbmodem123451"
BAUD = 9600
SEND_DELAY = 0.05  # seconds between each byte

ser = serial.Serial(PORT, BAUD, timeout=0.1)


def reader():
    while True:
        data = ser.read(ser.in_waiting or 1)
        if data:
            sys.stdout.write(data.decode("utf-8", errors="replace"))
            sys.stdout.flush()


t = threading.Thread(target=reader, daemon=True)
t.start()

print(f"Connected to {PORT} @ {BAUD}. Ctrl+C to quit.\n")


def slow_write(data: bytes):
    for byte in data:
        ser.write(bytes([byte]))
        time.sleep(SEND_DELAY)


try:
    while True:
        line = input()
        slow_write((line + "\n").encode())
except (KeyboardInterrupt, EOFError):
    print("\nClosing...")
finally:
    ser.close()
