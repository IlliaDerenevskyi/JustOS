#!/usr/bin/env python3
"""QEMU tests of the Stage 4 screens and of the Stage 3 screens that end the boot (level Q).

Usage: tests/qemu/run_observed_test.py <platform> <build-dir> <scenario>

These runs are watched from outside, because the code under test either never ends (the recovery diagnostics state
and the panic screen stop the processor and wait) or needs a person at the keyboard (the boot menu). The script
starts QEMU with a control socket (QMP), sends keys with "send-key" when the scenario needs them, takes a screen
dump with "screendump" (the picture that the virtual display really shows), and checks:

  - the serial log: the plain text lines that Stage 3 and Stage 4 wrote (and no "JP|RESULT" line when the
    processor must stop: a stopped boot must not look like a finished test);
  - the screen: every line that went through the Stage 3 text output is drawn at its place, pixel by pixel, with the
    font of the test kernel (an independent copy of the glyphs); only the text area is compared (the firmware
    console may have left text elsewhere);
  - for the menu: the screen of the menu itself shows the fields and the system information, and the values that
    the keys changed.

Scenarios:
  recovery        Stage 2 fails with a result that Stage 3 can recover from: Stage 4 enters the recovery diagnostics state.
  panic           Stage 2 fails with a result that Stage 3 cannot recover from: Stage 4 decides TERMINAL_HALT, Stage 3 shows the panic screen.
  menu_apply      The menu: edit the kernel parameters and three switches, F10; the kernel reports the frozen configuration.
  menu_recovery   The menu: switch "Recovery requested" on, F10; Stage 4 enters the recovery diagnostics state.

Exit status: 0 passed; 1 a check failed or a timeout; 2 a tool or file is missing.
"""

import json
import pathlib
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
WAIT_SECONDS = 60
KEY_PAUSE = 0.3
CELL_W, CELL_H = 8, 16
FIRST_GLYPH = 0x20
LIT = 127

# Characters that send-key knows by name (the others are lower-case letters and digits, which are their own names).
QCODES = {" ": "spc", "-": "minus", "=": "equal", ".": "dot", "/": "slash", ",": "comma", ";": "semicolon"}

RECOVERY_LINES = [
    r"^JustOS boot diagnostics$",
    r"^Status: RECOVERY$",
    r"^Error: JP_BOOT_FAILED in STAGE2$",
    r"^Severity: ERROR, UNKNOWN, source STAGE2$",
    r"^Events: 4$",
]

PANIC_LINES = [
    r"^JustOS boot failure$",
    r"^attempt: 1$",
    r"^stage: STAGE2$",
    r"^result: JP_BOOT_ERROR_INVALID_PARAM$",
    r"^severity: FATAL$",
    r"^recoverability: NON_RECOVERABLE$",
    r"^source: STAGE2$",
    r"^context: 0x00000000$",
]

MENU_RECOVERY_LINES = [
    r"^JustOS boot diagnostics$",
    r"^Status: RECOVERY$",
    r"^Error: none recorded$",
    r"^Events: 4$",
]

MENU_TEXTS = ["JustOS boot menu", "Kernel parameters:", "[x] Boot logging", "[ ] Verbose output", "[ ] Safe mode",
              "[ ] Recovery requested", "System information", "Boot status: NOT INITIALIZED",
              "Configuration: VALIDATED (DEFAULT)", "Menu: NOT_RUN", "Up/Down: field"]

SCENARIOS = {
    "recovery": dict(kernel="test_kernel_recovery.elf", interactive=False, keys=[], lines=RECOVERY_LINES, ends="halt"),
    "panic": dict(kernel="test_kernel_panic.elf", interactive=False, keys=[], lines=PANIC_LINES, ends="halt"),
    "menu_apply": dict(
        kernel="test_kernel.elf", interactive=True, ends="exit",
        # Enter starts editing, the text, Enter stops it; Down, Enter: boot logging off; Down, Enter: verbose on;
        # Down, Enter: safe mode on. Before F10 the screen is checked. The text has a space, "=" and "-" to check the
        # punctuation keys of the BIOS keyboard map as well.
        keys=["ret"] + [QCODES.get(c, c) for c in "quiet debug=1-x"] + ["ret", "down", "ret", "down", "ret", "down", "ret"],
        before_submit=["Kernel parameters: quiet debug=1-x", "[ ] Boot logging", "[x] Verbose output", "[x] Safe mode",
                       "[ ] Recovery requested"],
        submit=["f10"]),
    "menu_recovery": dict(
        kernel="test_kernel.elf", interactive=True, ends="halt", lines=MENU_RECOVERY_LINES, menu_cleared=True,
        keys=["down", "down", "down", "down", "ret"],
        before_submit=["[x] Recovery requested", "[x] Boot logging"],
        submit=["f10"]),
}


def fail(message):
    print("FAIL:", message)
    return 1


# ---------------------------------------------------------------- the screen

def load_font():
    """Parses tests/qemu/test_kernel/font8x16.c into {char code: 16 row bytes}."""
    text = (ROOT / "tests/qemu/test_kernel/font8x16.c").read_text()
    font = {}
    for rows, code in re.findall(r"\{ ((?:0x[0-9A-F]{2}(?:, )?){16}) \}, /\* 0x([0-9A-F]{2})", text):
        font[int(code, 16)] = [int(value, 16) for value in rows.split(", ")]
    if len(font) != 0x7F - FIRST_GLYPH:
        raise SystemExit("cannot read the font from font8x16.c")
    return font


class Screen:
    def __init__(self, path, font):
        data = pathlib.Path(path).read_bytes()
        match = re.match(rb"P6\s+(\d+)\s+(\d+)\s+(\d+)\s", data)
        if match is None or match.group(3) != b"255":
            raise SystemExit("the screen dump is not a P6 PPM with 255 levels")
        self.width, self.height = int(match.group(1)), int(match.group(2))
        self.pixels = data[match.end():]
        self.font = font
        self.columns = min(self.width // CELL_W, 80)

    def lit(self, x, y):
        o = (y * self.width + x) * 3
        return self.pixels[o] > LIT or self.pixels[o + 1] > LIT or self.pixels[o + 2] > LIT

    def cell_matches(self, row, column, char, inverse):
        glyph = self.font[ord(char)]
        for y in range(CELL_H):
            for x in range(CELL_W):
                want = ((glyph[y] & (0x80 >> x)) != 0) != inverse
                if self.lit(column * CELL_W + x, row * CELL_H + y) != want:
                    return False
        return True

    def text_at(self, row, column, text, inverse):
        if (row + 1) * CELL_H > self.height or column + len(text) > self.columns:
            return False
        return all(self.cell_matches(row, column + i, c, inverse) for i, c in enumerate(text))

    def has_text(self, text):
        """The text on the grid of cells, written normally or inverse (the current field of the menu is inverse)."""
        for row in range(self.height // CELL_H):
            for column in range(self.columns - len(text) + 1):
                if self.text_at(row, column, text, False) or self.text_at(row, column, text, True):
                    return True
        return False


# ---------------------------------------------------------------- QEMU control

class Qmp:
    def __init__(self, path):
        deadline = time.time() + 20
        self.client = None
        while time.time() < deadline:
            try:
                client = socket.socket(socket.AF_UNIX)
                client.connect(str(path))
                self.client = client
                break
            except OSError:
                client.close()
                time.sleep(0.2)
        if self.client is None:
            raise SystemExit("cannot connect to the QMP socket")
        self.stream = self.client.makefile("rw")
        json.loads(self.stream.readline())          # the greeting
        self.command("qmp_capabilities")

    def command(self, name, arguments=None):
        request = {"execute": name}
        if arguments is not None:
            request["arguments"] = arguments
        self.stream.write(json.dumps(request) + "\n")
        self.stream.flush()
        while True:
            reply = json.loads(self.stream.readline())
            if "return" in reply or "error" in reply:
                return reply

    def screendump(self, target):
        reply = self.command("screendump", {"filename": str(target), "format": "ppm"})
        return reply.get("error", {}).get("desc") if "error" in reply else None

    def send_key(self, qcode):
        self.command("send-key", {"keys": [{"type": "qcode", "data": qcode}], "hold-time": 60})

    def quit(self):
        try:
            self.command("quit")
        except (OSError, ValueError):
            pass


def read_log(path):
    if not path.exists():
        return []
    return path.read_text(errors="replace").replace("\r", "").split("\n")


# ---------------------------------------------------------------- the run

def build_image(platform, out, scenario, image):
    kernel = out / scenario["kernel"]
    suffix = "_interactive" if scenario["interactive"] else ""
    if platform == "x86_64-bios":
        parts = [out / "boot_sector.bin", out / ("loader%s.bin" % suffix), kernel]
        make = ["--boot-sector", str(parts[0]), "--loader", str(parts[1])]
    else:
        parts = [out / ("boot%s.efi" % suffix), kernel]
        make = ["--boot-app", str(parts[0])]
    for part in parts:
        if not part.is_file():
            print("not built yet:", part, file=sys.stderr)
            raise SystemExit(2)
    subprocess.run([str(ROOT / "scripts/make_image.sh"), platform, *make, "--kernel", str(kernel), "--out", str(image)],
                   check=True, stdout=subprocess.DEVNULL)


def wait_for_menu(qmp, dump, font):
    """Takes screen dumps until the title of the menu is drawn; returns the Screen, or None after the time limit."""
    deadline = time.time() + WAIT_SECONDS
    while time.time() < deadline:
        if qmp.screendump(dump) is None:
            screen = Screen(dump, font)
            if screen.text_at(0, 0, "JustOS boot menu", False):
                return screen
        time.sleep(0.7)
    return None


def check_screen_lines(screen, sent):
    """The lines that went through the Stage 3 text output, drawn at the top of the screen, pixel by pixel."""
    if len(sent) * CELL_H > screen.height:
        return fail("the screen is too small for the lines")
    wrong = 0
    checked = 0
    for y in range(len(sent) * CELL_H):
        line_index, row = divmod(y, CELL_H)
        text = sent[line_index]
        for x in range(screen.columns * CELL_W):
            checked += 1
            column, bit = divmod(x, CELL_W)
            want = column < len(text) and (screen.font[ord(text[column])][row] & (0x80 >> bit)) != 0
            if screen.lit(x, y) != want:
                wrong += 1
    if wrong:
        return fail("%d of %d pixels of the text area differ from the expected text (screen %dx%d)"
                    % (wrong, checked, screen.width, screen.height))
    return 0


def check_rest_is_black(screen, sent):
    """After the menu the whole screen was black before Stage 3 drew its lines: nothing is lit outside the lines."""
    lines_bottom = len(sent) * CELL_H
    text_right = screen.columns * CELL_W
    lit = 0
    for y in range(screen.height):
        for x in range(screen.width):
            if (y >= lines_bottom or x >= text_right) and screen.lit(x, y):
                lit += 1
    if lit:
        return fail("%d lit pixels outside the Stage 3 lines: the screen was not cleared after the menu" % lit)
    return 0


def main():
    if len(sys.argv) != 4 or sys.argv[3] not in SCENARIOS:
        print("usage: run_observed_test.py <platform> <build-dir> <%s>" % "|".join(SCENARIOS), file=sys.stderr)
        return 2
    platform, build, name = sys.argv[1], pathlib.Path(sys.argv[2]), sys.argv[3]
    scenario = SCENARIOS[name]
    out = build / "qemu" / platform
    image = out / ("disk_%s.img" % name)
    serial = out / ("serial_%s.log" % name)
    dump = out / ("screen_%s.ppm" % name)
    # A UNIX socket path is limited to about 107 characters, and a build directory can be deeper than that, so the
    # socket lives in its own short temporary directory.
    sock_dir = pathlib.Path(tempfile.mkdtemp(prefix="jp_"))
    sock = sock_dir / "qmp.sock"
    for stale in (dump, serial):            # an old log would end the wait at once
        stale.unlink(missing_ok=True)
    build_image(platform, out, scenario, image)
    font = load_font()

    run = [str(ROOT / "scripts/run_qemu.sh"), platform, "--image", str(image), "--nographic",
           "--timeout", str(WAIT_SECONDS * 2), "--serial-log", str(serial), "--qmp", str(sock)]
    process = subprocess.Popen(run, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    qmp = Qmp(sock)
    failures = 0

    try:
        if scenario["interactive"]:
            screen = wait_for_menu(qmp, dump, font)
            if screen is None:
                return fail("the menu did not appear in %d s" % WAIT_SECONDS)
            for text in MENU_TEXTS:
                if not screen.has_text(text):
                    failures += fail("the menu screen does not show: " + text)
            for qcode in scenario["keys"]:
                qmp.send_key(qcode)
                time.sleep(KEY_PAUSE)
            time.sleep(0.5)
            if qmp.screendump(dump) is not None:
                return fail("screendump failed")
            screen = Screen(dump, font)
            for text in scenario["before_submit"]:
                if not screen.has_text(text):
                    failures += fail("before F10 the menu screen does not show: " + text)
            for qcode in scenario["submit"]:
                qmp.send_key(qcode)
                time.sleep(KEY_PAUSE)

        if scenario["ends"] == "exit":
            try:
                process.wait(timeout=WAIT_SECONDS)
            except subprocess.TimeoutExpired:
                process.kill()
                return fail("the test kernel did not end the run in %d s" % WAIT_SECONDS)
            result = subprocess.run([str(ROOT / "tests/qemu/check_serial.sh"), platform, str(serial), name])
            return 1 if failures or result.returncode != 0 else 0

        # The processor stops: wait for the last line of the screen, then look at the display.
        patterns = scenario["lines"]
        deadline = time.time() + WAIT_SECONDS
        while time.time() < deadline:
            lines = read_log(serial)
            if any(re.match(patterns[-1], line) for line in lines):
                break
            if process.poll() is not None:
                break
            time.sleep(0.3)
        else:
            return fail("the screen of '%s' did not appear in %d s" % (name, WAIT_SECONDS))
        time.sleep(0.5)             # the screen is written before the serial line, but let the display catch up
        problem = qmp.screendump(dump)
        if problem is not None:
            return fail("screendump: " + problem)
        lines = read_log(serial)
    finally:
        qmp.quit()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
        shutil.rmtree(sock_dir, ignore_errors=True)

    if any(line.startswith("JP|RESULT") for line in lines):
        failures += fail("the log has a JP|RESULT line: a stopped boot must not look like a finished test")
    first = next((i for i, line in enumerate(lines) if re.match(patterns[0], line)), None)
    if first is None:
        return fail("the first line of the screen is missing: " + patterns[0])
    sent = [line for line in lines[first:] if line != ""]
    if len(sent) != len(patterns):
        failures += fail("expected %d lines from Stage 3 and Stage 4, the log has %d: %s" % (len(patterns), len(sent), sent))
    for pattern, line in zip(patterns, sent):
        if re.match(pattern, line) is None:
            failures += fail("line '%s' does not match %s" % (line, pattern))
    screen = Screen(dump, font)
    failures += check_screen_lines(screen, sent)
    if scenario.get("menu_cleared"):
        failures += check_rest_is_black(screen, sent)
    print("%s: %s %s (%dx%d, %d lines checked)" % (platform, name, "FAILED" if failures else "OK", screen.width, screen.height, len(sent)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
