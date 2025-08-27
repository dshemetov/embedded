# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pyserial",
# ]
# ///
import serial
import pathlib
import sys
import threading
import time

# import serial, pathlib, sys, threading, time


def reader(port):
    while True:
        try:
            line = port.readline()
            if line:
                print("Received:", line.decode(errors="replace").strip())
        except Exception as e:
            print("Reader error:", e)
            break


def main():
    port = serial.Serial("/dev/cu.usbserial-58900028661", 9600, timeout=0.1)
    time.sleep(2)  # Give the board time to reset

    # Start the reader thread
    thread = threading.Thread(target=reader, args=(port,), daemon=True)
    thread.start()

    # Send file contents
    with pathlib.Path(sys.argv[1]).open("rb") as f:
        while chunk := f.read(1024):
            port.write(chunk)
            time.sleep(0.1)  # Give Arduino time to echo back

    # Give the reader time to catch up before script exits
    time.sleep(2)


if __name__ == "__main__":
    main()
# while True:
#     input("Press Enter to send 'yo'...")
#     port.write(b"yo")
#     time.sleep(1)
#     a = port.readline()
#     if a:
#         print("Received:", a.decode(errors="replace").rstrip())
