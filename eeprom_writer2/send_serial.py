# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pyserial",
# ]
# ///
import sys
import threading
import time
import pathlib
import serial

SOF0 = 0xAA
SOF1 = 0x55
CMD_WRITE = 0x57
CMD_READ = 0x52
RPY_ACK = 0x41
RPY_DATA = 0x44


def reader(port: serial.Serial):
    while True:
        try:
            line = port.readline()
            if line:
                print("Received:", line.decode(errors="replace").strip())
        except Exception as e:
            print("Reader error:", e)
            break


def send_frame(
    port: serial.Serial, cmd: int, addr: int, len: int, data: bytes, checksum: int
):
    if addr > 0xFFFF:
        raise ValueError("Address currently restricted to 16 bits")
    frame = bytearray([SOF0, SOF1, cmd])
    frame.extend(addr.to_bytes(4, "little"))
    frame.extend(len.to_bytes(2, "little"))
    frame.extend(data)
    frame.extend(checksum.to_bytes(2, "little"))
    port.write(frame)
    port.flush()


def get_checksum(data: bytes):
    return sum(data) & 0xFFFF


def write_frame(port: serial.Serial, addr: int, data: bytes):
    checksum = get_checksum(data)
    print(f"Writing data with checksum: {checksum}")
    send_frame(port, CMD_WRITE, addr, len(data), data, checksum)


# TODO: I don't have a way to read from the EEPROM side aside from LEDs.
def read_frame(port: serial.Serial, addr: int, len: int):
    if addr > 0xFFFF:
        raise ValueError("Address currently restricted to 16 bits")
    send_frame(port, CMD_READ, addr, len, b"", 0)


def main():
    # Arg parse
    if len(sys.argv) < 2:
        print("Usage: python send_serial.py <device> [<file>]")
        sys.exit(1)
    elif len(sys.argv) == 2:
        device = sys.argv[1]
        file = None
    elif len(sys.argv) == 3:
        device = sys.argv[1]
        file = sys.argv[2]
    print(f"Using device: {device}")
    if file is not None:
        print(f"Using file: {file}")

    # Connect and start the reader thread
    port = serial.Serial(device, 115200, timeout=0.1)
    time.sleep(2)  # Give the board time to reset
    thread = threading.Thread(target=reader, args=(port,), daemon=True)
    thread.start()

    # Write data to the EEPROM.
    if file is None:
        # Send just a single frame
        write_frame(port, 0x000A, b"\xa5\x5a")
        # Wait
        time.sleep(1)
        # Read the data back from the EEPROM on the LEDs.
        read_frame(port, 0x000A, 2)
    else:
        # Send file contents
        with pathlib.Path(file).open("rb") as f:
            addr = 0x0
            while chunk := f.read(1024):
                write_frame(port, addr, chunk)
                addr += len(chunk)
                time.sleep(2)

    # Give the reader time to catch up before script exits
    time.sleep(2)
    # port.close()
    print("Done.")


if __name__ == "__main__":
    main()
