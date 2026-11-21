#!/usr/bin/env python3
"""QEMU test of the Stage 3 panic screen (level Q).

Usage: tests/qemu/run_panic_test.py <platform> <build-dir>

Runs build/qemu/<platform>/test_kernel_panic.elf: Stage 2 fails on purpose, the Stage 4 stand-in refuses the
recovery transition, so Stage 3 takes its terminal path, shows the panic screen, and stops the processor.
The processor never exits QEMU, so this script is the observer:

  1. it waits until the serial log holds the last panic line ("context: 0x...");
  2. it asks QEMU (QMP, "screendump") for the picture that the virtual display really shows;
  3. it stops QEMU and checks:
       - the serial log: the eight panic lines with the expected values, and no "JP|RESULT" line
         (a panic must not look like a finished test);
       - the screen: every line that Stage 3 sent is drawn at its place, pixel by pixel, with the font of
         the test kernel (an independent copy of the glyphs), and the rest of the screen is black.

Exit status: 0 passed; 1 a check failed or a timeout; 2 a tool or file is missing.
"""

import json
import pathlib
import re
import socket
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
WAIT_SECONDS = 60
CELL_W, CELL_H = 8, 16
FIRST_GLYPH = 0x20
LAST_LINE_PATTERN = re.compile(r"^context: 0x[0-9A-F]{8}$")

# The panic lines, in the order of the specification ("Panic, in detail" in milestone0.md).
PANIC_LINES = [
    r"^JustOS boot failure$",
    r"^attempt: 1$",
    r"^stage: STAGE2$",
    r"^result: JP_BOOT_FAILED$",
    r"^severity: ERROR$",
    r"^recoverability: UNKNOWN$",
    r"^source: STAGE2$",
    r"^context: 0x00000000$",
]


def fail(message):
    print("FAIL:", message)
    return 1


def load_font():
    """Parses tests/qemu/test_kernel/font8x16.c into {char code: 16 row bytes}."""
    text = (ROOT / "tests/qemu/test_kernel/font8x16.c").read_text()
    font = {}
    for rows, code in re.findall(r"\{ ((?:0x[0-9A-F]{2}(?:, )?){16}) \}, /\* 0x([0-9A-F]{2})", text):
        font[int(code, 16)] = [int(value, 16) for value in rows.split(", ")]
    if len(font) != 0x7F - FIRST_GLYPH:
        raise SystemExit("cannot read the font from font8x16.c")
    return font


def read_ppm(path):
    data = path.read_bytes()
    match = re.match(rb"P6\s+(\d+)\s+(\d+)\s+(\d+)\s", data)
    if match is None or match.group(3) != b"255":
        raise SystemExit("the screen dump is not a P6 PPM with 255 levels")
    width, height = int(match.group(1)), int(match.group(2))
    return width, height, data[match.end():]


def qmp_screendump(sock_path, target):
    deadline = time.time() + 20
    client = None
    while time.time() < deadline:
        try:
            client = socket.socket(socket.AF_UNIX)
            client.connect(str(sock_path))
            break
        except OSError:
            client.close()
            client = None
            time.sleep(0.2)
    if client is None:
        return "cannot connect to the QMP socket"
    stream = client.makefile("rw")

    def command(name, arguments=None):
        request = {"execute": name}
        if arguments is not None:
            request["arguments"] = arguments
        stream.write(json.dumps(request) + "\n")
        stream.flush()
        while True:
            reply = json.loads(stream.readline())
            if "return" in reply or "error" in reply:
                return reply

    json.loads(stream.readline())          # the greeting
    command("qmp_capabilities")
    reply = command("screendump", {"filename": str(target), "format": "ppm"})
    command("quit")
    client.close()
    return reply.get("error", {}).get("desc") if "error" in reply else None


def main():
    if len(sys.argv) != 3:
        print("usage: run_panic_test.py <platform> <build-dir>", file=sys.stderr)
        return 2
    platform, build = sys.argv[1], pathlib.Path(sys.argv[2])
    out = build / "qemu" / platform
    kernel = out / "test_kernel_panic.elf"
    if platform == "x86_64-bios":
        parts = [out / "boot_sector.bin", out / "loader.bin", kernel]
    else:
        parts = [out / "boot.efi", kernel]
    for part in parts:
        if not part.is_file():
            print("not built yet:", part, file=sys.stderr)
            return 2

    image = out / "disk_panic.img"
    serial = out / "serial_panic.log"
    dump = out / "screen_panic.ppm"
    sock = out / "qmp_panic.sock"
    for stale in (dump, sock, serial):      # an old log would end the wait at once
        stale.unlink(missing_ok=True)
    make = [str(ROOT / "scripts/make_image.sh"), platform, "--kernel", str(kernel), "--out", str(image)]
    if platform == "x86_64-bios":
        make += ["--boot-sector", str(out / "boot_sector.bin"), "--loader", str(out / "loader.bin")]
    else:
        make += ["--boot-app", str(out / "boot.efi")]
    subprocess.run(make, check=True, stdout=subprocess.DEVNULL)

    run = [str(ROOT / "scripts/run_qemu.sh"), platform, "--image", str(image), "--nographic",
           "--timeout", str(WAIT_SECONDS + 20), "--serial-log", str(serial), "--qmp", str(sock)]
    process = subprocess.Popen(run, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    deadline = time.time() + WAIT_SECONDS
    lines = []
    while time.time() < deadline:
        if serial.exists():
            lines = serial.read_text(errors="replace").replace("\r", "").split("\n")
            if any(LAST_LINE_PATTERN.match(line) for line in lines):
                break
        if process.poll() is not None:
            break
        time.sleep(0.3)
    else:
        process.kill()
        return fail("the panic screen did not appear in %d s" % WAIT_SECONDS)
    time.sleep(0.5)             # the screen is written before the serial line, but let the display catch up

    problem = qmp_screendump(sock, dump)
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
    if problem is not None:
        return fail("screendump: " + problem)
    lines = serial.read_text(errors="replace").replace("\r", "").split("\n")

    failures = 0
    if any(line.startswith("JP|RESULT") for line in lines):
        failures += fail("the log has a JP|RESULT line: a panic must not look like a finished test")
    first = next((i for i, line in enumerate(lines) if line == "JP|stage3.output=OK"), None)
    if first is None:
        return fail("the line JP|stage3.output=OK is missing: the Stage 3 output did not work")
    sent = [line for line in lines[first:] if line != ""]
    panic = sent[-len(PANIC_LINES):]
    for pattern, line in zip(PANIC_LINES, panic):
        if re.match(pattern, line) is None:
            failures += fail("panic line '%s' does not match %s" % (line, pattern))
    if len(sent) != 1 + len(PANIC_LINES):
        failures += fail("expected 1 + 8 lines from Stage 3, the log has %d: %s" % (len(sent), sent))

    # The screen: line k of the screen is the k-th message that Stage 3 sent.
    font = load_font()
    width, height, pixels = read_ppm(dump)
    columns = min(width // CELL_W, 80)
    expected_rows = len(sent) * CELL_H
    if expected_rows > height:
        return fail("the screen is too small for the lines")
    # Only the rectangle that Stage 3 owns is compared: the firmware console may have left its own text outside
    # it (Stage 3 draws its lines and does not clear the rest of the screen).
    wrong = 0
    checked = 0
    for y in range(expected_rows):
        for x in range(columns * CELL_W):
            checked += 1
            offset = (y * width + x) * 3
            lit = pixels[offset] > 127 or pixels[offset + 1] > 127 or pixels[offset + 2] > 127
            want = False
            line_index, row = divmod(y, CELL_H)
            column, bit = divmod(x, CELL_W)
            text = sent[line_index]
            if column < len(text):
                glyph = font[ord(text[column])]
                want = (glyph[row] & (0x80 >> bit)) != 0
            if lit != want:
                wrong += 1
    if wrong != 0:
        failures += fail("%d of %d pixels of the Stage 3 text area differ from the expected text (screen %dx%d)" % (wrong, checked, width, height))
    print("%s: panic screen %s (%dx%d, %d lines checked)" % (platform, "FAILED" if failures else "OK", width, height, len(sent)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
