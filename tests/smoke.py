"""Smoke test helper. Commands: start | probe | press <hexbuttons> [secs] | shot <png> | stop
Set PSYCHONAUTS_DIR to the game folder. Reads the running game's memory; writes
only the scripted pad file."""
import ctypes
import ctypes.wintypes as w
import os
import struct
import subprocess
import sys
import time
import zlib

GAME = os.environ["PSYCHONAUTS_DIR"]
PAD = GAME + r"\psychopadx_testpad.txt"
k32 = ctypes.WinDLL("kernel32", use_last_error=True)
u32 = ctypes.WinDLL("user32", use_last_error=True)
gdi = ctypes.WinDLL("gdi32")
u32.SetProcessDPIAware()
k32.OpenProcess.restype = w.HANDLE
k32.ReadProcessMemory.argtypes = [w.HANDLE, w.LPCVOID, w.LPVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]


def pid():
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Psychonauts.exe", "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    return int(out.split('","')[1]) if "Psychonauts" in out else None


def read(p, addr, n):
    h = k32.OpenProcess(0x10, False, p)
    buf = ctypes.create_string_buffer(n)
    got = ctypes.c_size_t()
    ok = k32.ReadProcessMemory(h, addr, buf, n, ctypes.byref(got))
    k32.CloseHandle(h)
    return buf.raw if ok else None


def game_window(p):
    found = []

    @ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    def cb(hwnd, _):
        owner = w.DWORD()
        u32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == p and u32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True

    u32.EnumWindows(cb, 0)
    return found[0] if found else None


CODES = {"A": 0x119, "B": 0x11a, "X": 0x118, "Y": 0x11b, "LT": 0x11c, "RT": 0x11d, "RB": 0x11e, "LB": 0x11f,
         "Start": 0x120, "Back": 0x121, "L3": 0x122, "R3": 0x123, "Up": 0x114, "Down": 0x115, "Left": 0x116, "Right": 0x117}


def probe():
    p = pid()
    if not p:
        print("not running")
        return
    print("pid", p, "foreground", u32.GetForegroundWindow() == game_window(p))
    print("XInput detected flag", read(p, 0x737230, 1)[0], "allowed", read(p, 0x731060, 1)[0])
    call = read(p, 0x405b65, 5)
    print("key lookup call target", hex(0x405b6a + struct.unpack("<i", call[1:])[0]), "(stock 0x405bb0)")
    states = read(p, 0x736460, 0x200)
    print("held:", [n for n, c in CODES.items() if states[c] & 1] or "none")


def shot(path):
    p = pid()
    hwnd = game_window(p)
    r = w.RECT()
    u32.GetClientRect(hwnd, ctypes.byref(r))
    pt = w.POINT(0, 0)
    u32.ClientToScreen(hwnd, ctypes.byref(pt))
    W, H = r.right, r.bottom
    sdc = u32.GetDC(0)
    mdc = gdi.CreateCompatibleDC(sdc)
    bmp = gdi.CreateCompatibleBitmap(sdc, W, H)
    gdi.SelectObject(mdc, bmp)
    gdi.BitBlt(mdc, 0, 0, W, H, sdc, pt.x, pt.y, 0x00CC0020)
    bi = struct.pack("<IiiHHIIiiII", 40, W, -H, 1, 32, 0, 0, 0, 0, 0, 0)
    buf = ctypes.create_string_buffer(W * H * 4)
    gdi.GetDIBits(mdc, bmp, 0, H, buf, bi, 0)
    rows = bytearray()
    raw = buf.raw
    for y in range(H):
        rows.append(0)
        row = raw[y * W * 4:(y + 1) * W * 4]
        rgb = bytearray(W * 3)
        rgb[0::3], rgb[1::3], rgb[2::3] = row[2::4], row[1::4], row[0::4]
        rows += rgb

    def chunk(t, c):
        return struct.pack(">I", len(c)) + t + c + struct.pack(">I", zlib.crc32(t + c))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(bytes(rows), 3)) + chunk(b"IEND", b""))
    print("shot", path, W, H, "foreground", u32.GetForegroundWindow() == hwnd)


def set_pad(line):
    with open(PAD, "w") as f:
        f.write(line)


cmd = sys.argv[1]
if cmd == "start":
    set_pad("0 0 0 0 0 0 0")
    subprocess.Popen([GAME + r"\Psychonauts.exe"], cwd=GAME)
elif cmd == "probe":
    probe()
elif cmd == "press":
    set_pad(f"{sys.argv[2]} 0 0 0 0 0 0")
    time.sleep(float(sys.argv[3]) if len(sys.argv) > 3 else 0.25)
    probe()
    set_pad("0 0 0 0 0 0 0")
    time.sleep(0.4)
elif cmd == "raw":
    set_pad(" ".join(sys.argv[2:]))
elif cmd == "shot":
    shot(sys.argv[2])
elif cmd == "focus":
    hwnd = game_window(pid())
    u32.ShowWindow(hwnd, 9)
    print("focus", u32.SetForegroundWindow(hwnd))
elif cmd == "stop":
    subprocess.run(["taskkill", "/IM", "Psychonauts.exe"])
