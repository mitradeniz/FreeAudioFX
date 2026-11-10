#!/usr/bin/env python3
"""Generates the artwork drawn across every amp's faceplate (shared/art/*.svg).

Each amp gets one scene on a 1200 x 400 canvas. The plugin scales it to cover
the whole amp head, anchored at the top, so the top band (behind the logo) and
the bottom band (behind the cabinet strip) show it fully, and it glows faintly
through the see-through faceplate in between. Keep the strongest focal shapes
around x = 705 (within about 85 px of it, above y = 115): that spot sits between the
logo and the brand on every amp. Keep the logo area (x < 600, y < 100) and the brand
area (x 790-1100, y 30-80) calm, and use the edges and the bottom for framing.

Only plain SVG is used (paths, basic shapes, gradients, opacity, transforms),
which the JUCE SVG renderer supports.

    python3 tools/art/make_amp_art.py            # writes shared/art/*.svg
"""

import math
import os
import random

W, H = 1200, 400
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "shared", "art")


# ----------------------------------------------------------------------------- helpers
class Svg:
    def __init__(self):
        self.defs, self.body, self.n = [], [], 0

    def uid(self, prefix):
        self.n += 1
        return f"{prefix}{self.n}"

    def lin(self, stops, x1=0, y1=0, x2=0, y2=1, units="objectBoundingBox"):
        gid = self.uid("l")
        st = "".join(f'<stop offset="{o}" stop-color="{c}" stop-opacity="{a}"/>' for o, c, a in stops)
        self.defs.append(f'<linearGradient id="{gid}" x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" gradientUnits="{units}">{st}</linearGradient>')
        return f"url(#{gid})"

    def rad(self, stops, cx=0.5, cy=0.5, r=0.5, units="objectBoundingBox"):
        gid = self.uid("r")
        st = "".join(f'<stop offset="{o}" stop-color="{c}" stop-opacity="{a}"/>' for o, c, a in stops)
        self.defs.append(f'<radialGradient id="{gid}" cx="{cx}" cy="{cy}" r="{r}" gradientUnits="{units}">{st}</radialGradient>')
        return f"url(#{gid})"

    def add(self, s):
        self.body.append(s)

    def render(self):
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">'
                f'<defs>{"".join(self.defs)}</defs>{"".join(self.body)}</svg>\n')


def f(v):
    return f"{v:.1f}".rstrip("0").rstrip(".")


def attrs(fill="none", stroke=None, sw=None, op=None, extra=""):
    a = f'fill="{fill}"'
    if stroke:
        a += f' stroke="{stroke}"'
    if sw is not None:
        a += f' stroke-width="{f(sw)}"'
    if op is not None:
        a += f' opacity="{f(op) if op >= 0.1 else round(op, 3)}"'
    return a + (" " + extra if extra else "")


def rect(s, x, y, w, h, fill, op=None, rx=0, stroke=None, sw=None):
    s.add(f'<rect x="{f(x)}" y="{f(y)}" width="{f(w)}" height="{f(h)}" rx="{f(rx)}" {attrs(fill, stroke, sw, op)}/>')


def circle(s, cx, cy, r, fill, op=None, stroke=None, sw=None):
    s.add(f'<circle cx="{f(cx)}" cy="{f(cy)}" r="{f(r)}" {attrs(fill, stroke, sw, op)}/>')


def ellipse(s, cx, cy, rx, ry, fill, op=None, stroke=None, sw=None, rot=0):
    t = f' transform="rotate({f(rot)} {f(cx)} {f(cy)})"' if rot else ""
    s.add(f'<ellipse cx="{f(cx)}" cy="{f(cy)}" rx="{f(rx)}" ry="{f(ry)}" {attrs(fill, stroke, sw, op)}{t}/>')


def path(s, d, fill="none", op=None, stroke=None, sw=None, extra=""):
    s.add(f'<path d="{d}" {attrs(fill, stroke, sw, op, extra)}/>')


def poly(pts, close=True):
    d = "M" + " L".join(f"{f(x)} {f(y)}" for x, y in pts)
    return d + (" Z" if close else "")


def line(s, x1, y1, x2, y2, stroke, sw=1, op=None, cap="round"):
    s.add(f'<line x1="{f(x1)}" y1="{f(y1)}" x2="{f(x2)}" y2="{f(y2)}" stroke="{stroke}" stroke-width="{f(sw)}" '
          f'stroke-linecap="{cap}"{(" opacity=" + chr(34) + f(op) + chr(34)) if op is not None else ""}/>')


def star_pts(cx, cy, n, ro, ri, rot=-90):
    pts = []
    for i in range(n * 2):
        a = math.radians(rot + i * 180 / n)
        r = ro if i % 2 == 0 else ri
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


def glow(s, cx, cy, r, colour, alpha=0.6):
    circle(s, cx, cy, r, s.rad([(0, colour, alpha), (0.45, colour, alpha * 0.35), (1, colour, 0)]))


def sky(s, stops):
    rect(s, 0, 0, W, H, s.lin(stops))


def starfield(s, n, seed, y0=0, y1=H, colour="#ffffff", rmax=1.6, alpha=(0.3, 0.95)):
    rng = random.Random(seed)
    for _ in range(n):
        x, y = rng.uniform(0, W), rng.uniform(y0, y1)
        r = rng.uniform(0.4, rmax)
        circle(s, x, y, r, colour, rng.uniform(*alpha))
    for _ in range(max(2, n // 40)):  # a few bright twinkles
        x, y = rng.uniform(40, W - 40), rng.uniform(y0, y1)
        path(s, poly(star_pts(x, y, 4, rng.uniform(5, 9), 1.2)), colour, 0.9)


def mountains(s, base, peaks, fill, op=None, seed=1, jag=0.0):
    rng = random.Random(seed)
    pts = [(-10, H + 10), (-10, base)]
    for x, y in peaks:
        if jag:
            pts.append((x - rng.uniform(10, 30), y + rng.uniform(jag * 0.3, jag)))
        pts.append((x, y))
    pts += [(W + 10, base), (W + 10, H + 10)]
    path(s, poly(pts), fill, op)


def hills(s, y, amp, wl, fill, op=None, phase=0.0):
    d = f"M-10 {H + 10} L-10 {f(y)}"
    for x in range(0, W + 30, 20):
        d += f" L{x} {f(y + amp * math.sin((x / wl) * 2 * math.pi + phase))}"
    d += f" L{W + 10} {H + 10} Z"
    path(s, d, fill, op)


def wave_line(y, amp, wl, phase=0.0, x0=-10, x1=W + 10, step=12):
    d = ""
    for i, x in enumerate(range(x0, x1 + step, step)):
        yy = y + amp * math.sin(x / wl * 2 * math.pi + phase)
        d += (" L" if i else "M") + f"{f(x)} {f(yy)}"
    return d


def palm(s, x, y, h, fill, op=None, lean=0.15, flip=1):
    tx = x + flip * lean * h
    d = (f"M{f(x - 7)} {f(y)} Q{f(x + flip * lean * h * 0.4)} {f(y - h * 0.5)} {f(tx - 3)} {f(y - h)} "
         f"L{f(tx + 3)} {f(y - h)} Q{f(x + flip * lean * h * 0.4 + 8)} {f(y - h * 0.5)} {f(x + 7)} {f(y)} Z")
    path(s, d, fill, op)
    for ang, ln in ((-160, 1.0), (-130, 0.9), (-95, 0.7), (-60, 0.95), (-25, 1.0), (10, 0.8), (-200, 0.8)):
        a = math.radians(ang)
        L = h * 0.45 * ln
        ex, ey = tx + L * math.cos(a), y - h + L * math.sin(a) + L * 0.35
        cx, cy = tx + L * 0.5 * math.cos(a), y - h + L * 0.5 * math.sin(a) - 10
        nx, ny = -math.sin(a) * 9, math.cos(a) * 9
        path(s, f"M{f(tx)} {f(y - h)} Q{f(cx + nx)} {f(cy + ny)} {f(ex)} {f(ey)} Q{f(cx - nx)} {f(cy - ny)} {f(tx)} {f(y - h)} Z", fill, op)


def dead_tree(s, x, y, h, fill, seed, op=None):
    rng = random.Random(seed)

    def branch(x0, y0, ang, length, width, depth):
        x1 = x0 + length * math.cos(ang)
        y1 = y0 + length * math.sin(ang)
        line(s, x0, y0, x1, y1, fill, width, op)
        if depth > 0:
            for _ in range(2 if depth > 1 else 3):
                branch(x1, y1, ang + rng.uniform(-0.75, 0.75), length * rng.uniform(0.55, 0.78), width * 0.62, depth - 1)

    branch(x, y, -math.pi / 2 + rng.uniform(-0.12, 0.12), h * 0.45, h * 0.06, 4)


def flame_path(x, y, w, h, lean=0.0):
    return (f"M{f(x - w / 2)} {f(y)} C{f(x - w * 0.6)} {f(y - h * 0.45)} {f(x - w * 0.1 + lean)} {f(y - h * 0.55)} "
            f"{f(x + lean)} {f(y - h)} C{f(x + w * 0.25 + lean)} {f(y - h * 0.6)} {f(x + w * 0.65)} {f(y - h * 0.4)} "
            f"{f(x + w / 2)} {f(y)} Z")


def bolt(points_seed, x, y, h, width):
    rng = random.Random(points_seed)
    pts, cx, cy = [(x, y)], x, y
    while cy < y + h:
        cx += rng.uniform(-width, width)
        cy += rng.uniform(h * 0.08, h * 0.18)
        pts.append((cx, min(cy, y + h)))
    return pts


def music_note(s, x, y, size, fill, op=None, double=False):
    ellipse(s, x, y, size * 0.42, size * 0.3, fill, op, rot=-25)
    rect(s, x + size * 0.32, y - size * 1.6, size * 0.09, size * 1.6, fill, op)
    if double:
        ellipse(s, x + size * 0.9, y - size * 0.25, size * 0.42, size * 0.3, fill, op, rot=-25)
        rect(s, x + size * 1.22, y - size * 1.85, size * 0.09, size * 1.6, fill, op)
        path(s, poly([(x + size * 0.36, y - size * 1.6), (x + size * 1.31, y - size * 1.85),
                      (x + size * 1.31, y - size * 1.6), (x + size * 0.36, y - size * 1.35)]), fill, op)
    else:
        path(s, f"M{f(x + size * 0.41)} {f(y - size * 1.6)} q{f(size * 0.5)} {f(size * 0.3)} {f(size * 0.35)} {f(size * 0.9)}",
             "none", op, fill, size * 0.12)


# ----------------------------------------------------------------------------- scenes
def dream():
    s = Svg()
    sky(s, [(0, "#090720", 1), (0.55, "#241a52", 1), (1, "#5a3f93", 1)])
    for cx, cy, r, c, a in ((300, 120, 320, "#ff7ad9", 0.20), (900, 260, 380, "#56b8ff", 0.20), (620, 40, 240, "#b49bff", 0.25)):
        glow(s, cx, cy, r, c, a)
    starfield(s, 260, 3)
    # crescent moon
    glow(s, 705, 70, 110, "#fff4d6", 0.35)
    circle(s, 705, 70, 50, "#fff3cf")
    circle(s, 728, 56, 46, "#1c1544")
    # ringed planet
    circle(s, 1130, 160, 34, s.lin([(0, "#ffd1f0", 1), (1, "#a26bd6", 1)]))
    ellipse(s, 1130, 164, 70, 14, "none", 0.9, "#f8d9ff", 3, rot=-14)
    circle(s, 140, 330, 22, s.lin([(0, "#9fe7ff", 1), (1, "#3c6fd1", 1)]), 0.9)
    # shooting star
    path(s, "M430 60 L560 22", "none", 0.85, s.lin([(0, "#ffffff", 0), (1, "#ffffff", 1)], 0, 0, 1, 0), 2.2)
    # dreamy clouds at the bottom
    rng = random.Random(9)
    for i in range(46):
        x = rng.uniform(-40, W + 40)
        y = rng.uniform(330, 410)
        circle(s, x, y, rng.uniform(30, 70), "#f3e9ff", rng.uniform(0.10, 0.26))
    for i in range(10):
        circle(s, rng.uniform(0, W), rng.uniform(150, 300), rng.uniform(40, 90), "#e9dcff", 0.05)
    return s


def dub():
    s = Svg()
    sky(s, [(0, "#0e2414", 1), (1, "#081a0f", 1)])
    cx, cy = 640, 430
    cols = ["#e8b417", "#c5281c", "#1f8f3a"]
    for i in range(36):
        a0 = math.radians(180 + i * 5)
        a1 = math.radians(180 + i * 5 + 5)
        R = 900
        path(s, poly([(cx, cy), (cx + R * math.cos(a0), cy + R * math.sin(a0)), (cx + R * math.cos(a1), cy + R * math.sin(a1))]),
             cols[i % 3], 0.22)
    glow(s, 640, 400, 260, "#ffd23f", 0.45)
    circle(s, 640, 410, 120, "#ffcf3a", 0.9)
    for r, c in ((96, "#e8a417"), (70, "#d4421c")):
        circle(s, 640, 410, r, c, 0.55)
    # sound system stacks on both sides
    for x0, flip in ((40, 1), (1010, -1)):
        for row in range(3):
            for col in range(2):
                bx, by = x0 + col * 78, 150 + row * 84
                rect(s, bx, by, 72, 78, "#151515", 0.95, 4, "#2c2c2c", 2)
                circle(s, bx + 36, by + 39, 27, "#2b2b2b", None, "#5a5a5a", 2)
                circle(s, bx + 36, by + 39, 11, "#0d0d0d")
                circle(s, bx + 36, by + 39, 4, "#777777")
    for x, fl in ((240, 1), (960, -1)):
        palm(s, x, 410, 230, "#06120a", 0.95, 0.18, fl)
    starfield(s, 60, 5, 0, 140, "#fff6c8", 1.2)
    return s


def spacedelay():
    s = Svg()
    sky(s, [(0, "#05070f", 1), (0.6, "#0c1a33", 1), (1, "#16335e", 1)])
    starfield(s, 220, 11)
    glow(s, 1110, 300, 260, "#2f7fd6", 0.35)
    circle(s, 1110, 300, 80, s.lin([(0, "#7fc3ff", 1), (1, "#1b3f7a", 1)]))
    ellipse(s, 1110, 308, 165, 28, "none", 0.85, "#b9dcff", 5, rot=-12)
    ellipse(s, 1110, 308, 145, 23, "none", 0.5, "#e8f4ff", 2, rot=-12)
    for i, (rx, ry) in enumerate(((420, 120), (560, 170), (700, 220))):
        ellipse(s, 600, 330, rx, ry, "none", 0.18, "#9cc8ff", 1.5, rot=-6)
    # rocket
    rx, ry = 705, 62
    path(s, f"M{rx} {ry - 40} C{rx + 16} {ry - 20} {rx + 16} {ry + 22} {rx + 10} {ry + 34} L{rx - 10} {ry + 34} "
            f"C{rx - 16} {ry + 22} {rx - 16} {ry - 20} {rx} {ry - 40} Z", "#e8eef5", None, "#8aa0b8", 1.5)
    circle(s, rx, ry - 6, 7, "#2f7fd6", None, "#ffffff", 2)
    path(s, poly([(rx - 10, ry + 18), (rx - 24, ry + 40), (rx - 10, ry + 34)]), "#d6452f")
    path(s, poly([(rx + 10, ry + 18), (rx + 24, ry + 40), (rx + 10, ry + 34)]), "#d6452f")
    path(s, flame_path(rx, ry + 34, 16, -44), s.lin([(0, "#ffe27a", 1), (1, "#ff5a1f", 0)]))
    path(s, f"M{rx - 60} {ry + 160} Q{rx - 20} {ry + 80} {rx} {ry + 70}", "none", 0.35, "#9cc8ff", 2, 'stroke-dasharray="4 8"')
    # tape reels in the lower corners (it is a tape echo)
    for x in (90,):
        circle(s, x, 360, 60, "#0f1a2c", 0.9, "#4f7fb8", 3)
        for k in range(3):
            a = math.radians(k * 120 - 90)
            circle(s, x + 32 * math.cos(a), 360 + 32 * math.sin(a), 15, "#05070f")
        circle(s, x, 360, 9, "#9cc8ff")
    return s


def goat_skull(s, cx, cy, sc, bone, dark):
    """Front view of a horned goat skull, centred on (cx, cy)."""
    def P(x, y):
        return f"{f(cx + x * sc)} {f(cy + y * sc)}"

    for side in (-1, 1):
        k = side
        # horn: thick at the skull, sweeping up, out and curling down
        path(s, f"M{P(k * 14, -38)} C{P(k * 30, -110)} {P(k * 120, -120)} {P(k * 150, -60)} "
                f"C{P(k * 168, -20)} {P(k * 150, 20)} {P(k * 118, 28)} "
                f"C{P(k * 138, 8)} {P(k * 140, -30)} {P(k * 122, -58)} "
                f"C{P(k * 100, -90)} {P(k * 52, -84)} {P(k * 36, -30)} Z", bone, None, dark, 2 * sc)
        for t in range(6):
            u = 0.12 + t * 0.13
            hx, hy = k * (22 + 125 * u), -60 - 55 * math.sin(u * math.pi * 0.95)
            path(s, f"M{P(hx - k * 6, hy - 8)} L{P(hx + k * 8, hy + 10)}", "none", 0.75, dark, 2.2 * sc)
        # ear
        path(s, f"M{P(k * 36, -18)} C{P(k * 70, -26)} {P(k * 86, -8)} {P(k * 74, 4)} C{P(k * 60, 6)} {P(k * 44, 0)} {P(k * 34, -2)} Z",
             bone, None, dark, 1.6 * sc)
    # skull
    path(s, f"M{P(-40, -34)} C{P(-46, -4)} {P(-36, 18)} {P(-28, 30)} C{P(-22, 52)} {P(-18, 74)} {P(-14, 92)} "
            f"C{P(-8, 100)} {P(8, 100)} {P(14, 92)} C{P(18, 74)} {P(22, 52)} {P(28, 30)} "
            f"C{P(36, 18)} {P(46, -4)} {P(40, -34)} C{P(26, -52)} {P(-26, -52)} {P(-40, -34)} Z", bone, None, dark, 2 * sc)
    for side in (-1, 1):
        path(s, f"M{P(side * 8, -10)} C{P(side * 18, -22)} {P(side * 34, -20)} {P(side * 34, -8)} "
                f"C{P(side * 34, 6)} {P(side * 20, 12)} {P(side * 10, 2)} Z", dark)
    path(s, f"M{P(0, 46)} L{P(-7, 64)} L{P(0, 70)} L{P(7, 64)} Z", dark)
    path(s, f"M{P(0, -40)} L{P(0, 36)}", "none", 0.5, dark, 1.5 * sc)
    for y in (84, 90):
        path(s, f"M{P(-10, y)} L{P(10, y)}", "none", 0.6, dark, 1.2 * sc)


def blackmetal():
    s = Svg()
    sky(s, [(0, "#020203", 1), (0.6, "#0d0f14", 1), (1, "#1b1e26", 1)])
    starfield(s, 120, 13, 0, 220, "#c9d6e3", 1.1, (0.15, 0.6))
    glow(s, 705, 74, 200, "#9fc3d9", 0.25)
    circle(s, 705, 74, 66, s.rad([(0, "#e6eef4", 1), (1, "#9aa9b6", 1)]), 0.95)
    for cx, cy, r in ((680, 50, 12), (735, 96, 9), (700, 112, 6), (740, 44, 5)):
        circle(s, cx, cy, r, "#8a97a3", 0.45)
    # big summoning circle behind everything
    cx, cy, R = 705, 205, 205
    circle(s, cx, cy, R, "none", 0.30, "#9fc3d9", 2.2)
    circle(s, cx, cy, R - 14, "none", 0.20, "#9fc3d9", 1.2)
    pts = [(cx + (R - 14) * math.sin(math.radians(180 + k * 144)), cy - (R - 14) * math.cos(math.radians(180 + k * 144))) for k in range(5)]
    path(s, poly(pts), "none", 0.26, "#9fc3d9", 2)
    for k in range(24):
        a = math.radians(k * 15)
        line(s, cx + (R - 7) * math.cos(a), cy + (R - 7) * math.sin(a), cx + (R + 6) * math.cos(a), cy + (R + 6) * math.sin(a), "#9fc3d9", 1.4, 0.3)
    goat_skull(s, 705, 80, 0.62, "#0b0c0f", "#c9d3db")
    # dead forest
    rng = random.Random(4)
    for i in range(16):
        x = rng.choice([rng.uniform(-20, 320), rng.uniform(880, W + 20)])
        dead_tree(s, x, 420, rng.uniform(170, 300), "#050506", i + 3, 0.95)
    hills(s, 370, 10, 300, "#050506")
    for y, a in ((330, 0.12), (355, 0.18)):
        rect(s, 0, y, W, 50, s.lin([(0, "#9fb4c4", 0), (0.5, "#9fb4c4", a), (1, "#9fb4c4", 0)], 0, 0, 1, 0))
    return s


def warmrock():
    s = Svg()
    sky(s, [(0, "#1d0a05", 1), (0.45, "#6e1d0e", 1), (0.8, "#c8501c", 1), (1, "#f2a541", 1)])
    glow(s, 640, 300, 420, "#ffb347", 0.45)
    circle(s, 640, 300, 110, s.lin([(0, "#ffe9a8", 1), (1, "#ff8a2a", 1)]))
    starfield(s, 70, 21, 0, 120, "#ffe0b0", 1.2)
    # vinyl record rising behind the hills
    vx, vy = 1120, 290
    circle(s, vx, vy, 120, "#120807", 0.92)
    for r in range(40, 118, 7):
        circle(s, vx, vy, r, "none", 0.35, "#5a3020", 1)
    circle(s, vx, vy, 34, "#d2452b")
    circle(s, vx, vy, 5, "#120807")
    path(s, f"M{vx - 100} {vy - 50} A110 110 0 0 1 {vx - 40} {vy - 105}", "none", 0.35, "#ffffff", 3)
    # winged guitar pick, top centre
    px, py = 705, 58
    for side in (-1, 1):
        for k in range(5):
            L = 78 - k * 11
            a = math.radians(-14 + k * 11)
            x2 = px + side * (30 + L * math.cos(a))
            y2 = py - 6 + L * math.sin(a) * 0.55
            path(s, f"M{f(px + side * 24)} {f(py - 10 + k * 5)} Q{f(px + side * (40 + L * 0.5))} {f(py - 26 + k * 7)} {f(x2)} {f(y2)} "
                    f"Q{f(px + side * (36 + L * 0.45))} {f(py + 2 + k * 6)} {f(px + side * 22)} {f(py + 2 + k * 5)} Z",
                 "#f2d79a" if k % 2 == 0 else "#e0b45e", 0.95, "#5a2a10", 1.2)
    path(s, f"M{px} {py + 44} C{px - 30} {py + 20} {px - 40} {py - 18} {px - 26} {py - 32} C{px - 12} {py - 44} {px + 12} {py - 44} "
            f"{px + 26} {py - 32} C{px + 40} {py - 18} {px + 30} {py + 20} {px} {py + 44} Z", "#d2452b", None, "#ffe9a8", 3)
    path(s, poly(star_pts(px, py - 4, 5, 15, 6)), "#ffe9a8")
    hills(s, 330, 18, 420, "#3a1208", None, 0.6)
    hills(s, 355, 12, 260, "#210904", None, 2.1)
    # amp stacks silhouettes on the hill
    for x in (120, 200):
        rect(s, x, 250, 70, 50, "#120504"); rect(s, x, 302, 70, 60, "#120504")
        for yy in (258, 312):
            rect(s, x + 6, yy, 58, 6, "#2a0e07")
    # flames along the bottom edge
    rng = random.Random(8)
    for i in range(40):
        x = rng.uniform(-10, W + 10)
        path(s, flame_path(x, 410, rng.uniform(26, 50), rng.uniform(40, 90), rng.uniform(-8, 8)),
             s.lin([(0, "#ffd36a", 0.9), (1, "#ff4a1a", 0.0)], 0, 1, 0, 0), 0.6)
    return s


def surf():
    s = Svg()
    sky(s, [(0, "#2f6d74", 1), (0.5, "#8fd0c3", 1), (1, "#ffd9a8", 1)])
    glow(s, 700, 140, 260, "#fff1c4", 0.6)
    circle(s, 700, 140, 70, "#ffe08a")
    for i in range(5):
        rect(s, 630, 112 + i * 14, 140, 4, "#ffcf6a", 0.8)
    # big curling wave from the left
    path(s, "M-20 410 L-20 220 C80 120 260 110 340 190 C380 230 330 270 290 250 C330 230 300 190 250 200 "
            "C180 215 160 300 260 330 C420 370 700 330 900 350 L1220 360 L1220 410 Z", "#1f6f88")
    path(s, "M-20 230 C80 135 250 125 330 195", "none", 0.9, "#e8fbff", 6)
    path(s, "M260 330 C420 370 700 330 900 350 L1220 360", "none", 0.8, "#e8fbff", 4)
    for k in range(4):
        path(s, wave_line(372 + k * 9, 4, 90, k), "none", 0.5, "#e8fbff", 2)
    rng = random.Random(3)
    for i in range(40):
        circle(s, rng.uniform(170, 360), rng.uniform(120, 230), rng.uniform(1.5, 4.5), "#ffffff", 0.8)
    for x, fl in ((1000, -1), (1110, -1)):
        palm(s, x, 380, 240 if x == 1000 else 190, "#1d3b37", 0.95, 0.2, fl)
    # surfboards
    for x, c in ((900, "#e8572c"), (940, "#ffd56b")):
        ellipse(s, x, 300, 16, 70, c, None, "#7a3416", 2, rot=8)
        line(s, x - 2, 240, x + 6, 360, "#ffffff", 2, 0.7)
    return s


def shoegaze():
    s = Svg()
    sky(s, [(0, "#2a1530", 1), (0.5, "#7b3f6e", 1), (1, "#e3a5c7", 1)])
    rng = random.Random(17)
    cols = ["#ffb3d9", "#b48bff", "#8fd3ff", "#ffd1a6", "#ff8fb8"]
    for i in range(60):
        glow(s, rng.uniform(-50, W + 50), rng.uniform(-30, H + 30), rng.uniform(30, 150), rng.choice(cols), rng.uniform(0.12, 0.35))
    for i in range(14):
        cx, cy, R = rng.uniform(0, W), rng.uniform(0, H), rng.uniform(20, 46)
        for k in range(6):
            a = math.radians(k * 60 + rng.uniform(0, 30))
            ellipse(s, cx + R * 0.6 * math.cos(a), cy + R * 0.6 * math.sin(a), R * 0.55, R * 0.28, "#ffe3f1", 0.18,
                    rot=math.degrees(a))
        circle(s, cx, cy, R * 0.18, "#fff3b0", 0.3)
    for k in range(6):
        path(s, wave_line(60 + k * 60, 14, 380, k * 1.3), "none", 0.18, "#ffffff", 3)
    return s


def arena():
    s = Svg()
    sky(s, [(0, "#05040a", 1), (1, "#1a0c22", 1)])
    for x, ang, c in ((180, 25, "#ff2fa6"), (420, 10, "#4fd8ff"), (780, -10, "#ff2fa6"), (1020, -25, "#ffe14f"), (600, 0, "#ffffff")):
        a = math.radians(ang)
        tx, ty = x + 520 * math.sin(a), 520
        path(s, poly([(x - 8, -10), (x + 8, -10), (tx + 120, ty), (tx - 120, ty)]),
             s.lin([(0, c, 0.55), (1, c, 0.0)]), 0.8)
        circle(s, x, 0, 14, c, 0.9)
    for k in range(12):
        line(s, 600, 20, 600 + math.cos(math.radians(200 + k * 12)) * 900, 20 - math.sin(math.radians(200 + k * 12)) * 900,
             "#ff2fa6" if k % 2 else "#4fd8ff", 1, 0.25)
    rng = random.Random(6)
    # crowd with raised hands
    d = f"M-10 {H + 10} L-10 360"
    x = -10
    while x < W + 20:
        x += rng.uniform(12, 22)
        d += f" Q{f(x - 8)} {f(338 - rng.uniform(0, 14))} {f(x)} {f(360 - rng.uniform(0, 8))}"
    d += f" L{W + 10} {H + 10} Z"
    path(s, d, "#020203")
    for i in range(30):
        hx = rng.uniform(10, W - 10)
        hy = rng.uniform(300, 340)
        line(s, hx, 370, hx + rng.uniform(-10, 10), hy, "#020203", 6)
        if rng.random() < 0.5:
            path(s, poly([(hx - 7, hy), (hx - 4, hy - 12), (hx, hy - 4), (hx + 4, hy - 12), (hx + 7, hy)]), "#020203")
    starfield(s, 50, 2, 0, 300, "#ffffff", 1.3)
    return s


def stereojazz():
    s = Svg()
    sky(s, [(0, "#07080c", 1), (0.6, "#14171f", 1), (1, "#262a35", 1)])
    starfield(s, 90, 31, 0, 200, "#dfe6ee", 1.1, (0.2, 0.7))
    glow(s, 705, 52, 140, "#e8eef5", 0.25)
    circle(s, 705, 52, 34, "#eef2f6")
    circle(s, 721, 42, 30, "#0b0d12")
    rng = random.Random(12)
    x = -10
    while x < W + 10:
        w, h = rng.uniform(40, 90), rng.uniform(80, 220)
        rect(s, x, H - h, w, h + 5, "#0a0b10")
        for wy in range(int(H - h + 12), H - 10, 16):
            for wx in range(int(x + 8), int(x + w - 8), 14):
                if rng.random() < 0.28:
                    rect(s, wx, wy, 6, 8, "#ffd88a", rng.uniform(0.35, 0.9))
        x += w + rng.uniform(2, 10)
    for x, y in ((630, 108), (650, 70), (765, 100), (785, 62), (1150, 150), (60, 160)):
        music_note(s, x, y, rng.uniform(12, 18), "#c9d2db", rng.uniform(0.45, 0.8), rng.random() < 0.5)
    for k in range(4):
        path(s, wave_line(150 + k * 40, 20, 500, k), "none", 0.06, "#ffffff", 18)
    return s


def doom():
    s = Svg()
    sky(s, [(0, "#0c0604", 1), (0.6, "#2a120a", 1), (1, "#4c1f0e", 1)])
    glow(s, 705, 100, 300, "#ff7a1a", 0.45)
    circle(s, 705, 100, 72, "#ffb35a")
    circle(s, 705, 100, 66, "#120806")
    rng = random.Random(14)
    for i in range(140):
        circle(s, rng.uniform(0, W), rng.uniform(0, H), rng.uniform(0.6, 2.2), rng.choice(["#ff9a4a", "#7a6a60"]), rng.uniform(0.2, 0.7))
    # volcanoes
    for vx, base, top, w in ((200, 410, 170, 340), (1010, 410, 150, 380)):
        path(s, poly([(vx - w, base), (vx - 40, top), (vx + 40, top), (vx + w, base)]), "#160a06")
        glow(s, vx, top, 90, "#ff6a1a", 0.6)
        for k in range(3):
            lx = vx + rng.uniform(-30, 30)
            path(s, f"M{f(lx)} {f(top)} Q{f(lx + rng.uniform(-60, 60))} {f(top + 110)} {f(lx + rng.uniform(-120, 120))} {f(base)}",
                 "none", 0.85, "#ff6a1a", rng.uniform(3, 6))
        for k in range(8):
            path(s, flame_path(vx + rng.uniform(-30, 30), top, rng.uniform(12, 24), rng.uniform(30, 70)), "#ffb35a", 0.7)
    mountains(s, 410, [(500, 300), (620, 330), (760, 290), (880, 340)], "#0c0503")
    for k in range(10):
        x0 = rng.uniform(0, W)
        path(s, f"M{f(x0)} 400 l{f(rng.uniform(-30, 30))} -{f(rng.uniform(10, 30))} l{f(rng.uniform(-20, 20))} -{f(rng.uniform(8, 20))}",
             "none", 0.8, "#ff6a1a", 2)
    return s


def drive():
    s = Svg()
    sky(s, [(0, "#060607", 1), (0.6, "#140b08", 1), (1, "#3a1307", 1)])
    rng = random.Random(23)
    glow(s, 640, 430, 520, "#ff5a1f", 0.55)
    for layer, (col, hmin, hmax, a) in enumerate((("#7a1505", 120, 230, 0.8), ("#d63a0f", 90, 180, 0.8), ("#ff8a1f", 60, 130, 0.85), ("#ffd25a", 30, 70, 0.9))):
        for i in range(36):
            x = rng.uniform(-20, W + 20)
            path(s, flame_path(x, 410, rng.uniform(40, 80) * (1 - layer * 0.15), rng.uniform(hmin, hmax), rng.uniform(-20, 20)), col, a)
    for i in range(70):
        x, y = rng.uniform(0, W), rng.uniform(40, 330)
        circle(s, x, y, rng.uniform(0.8, 2.4), rng.choice(["#ffb347", "#ff5a1f", "#ffe08a"]), rng.uniform(0.4, 0.95))
    for x in (705, 1150):
        pts = bolt(int(x), x, -10, 150, 26)
        path(s, poly(pts, False), "none", 0.35, "#ff8a3a", 9)
        path(s, poly(pts, False), "none", 0.95, "#fff1d0", 2.4)
    return s


def clean():
    s = Svg()
    sky(s, [(0, "#1e1814", 1), (0.6, "#2c241e", 1), (1, "#3b3027", 1)])
    glow(s, 705, 70, 260, "#15a99a", 0.3)
    # faceted gem
    cx, cy = 705, 64
    top = [(cx - 70, cy - 10), (cx - 40, cy - 40), (cx + 40, cy - 40), (cx + 70, cy - 10)]
    path(s, poly(top + [(cx, cy + 70)]), "#bff4ec", 0.9, "#ffffff", 1.5)
    for a, b, c, col in (((cx - 70, cy - 10), (cx - 40, cy - 40), (cx - 20, cy - 10), "#8fe6da"),
                         ((cx - 20, cy - 10), (cx, cy - 40), (cx + 20, cy - 10), "#d9fbf6"),
                         ((cx + 20, cy - 10), (cx + 40, cy - 40), (cx + 70, cy - 10), "#8fe6da"),
                         ((cx - 70, cy - 10), (cx - 20, cy - 10), (cx, cy + 70), "#56c7b8"),
                         ((cx - 20, cy - 10), (cx + 20, cy - 10), (cx, cy + 70), "#e8fffb"),
                         ((cx + 20, cy - 10), (cx + 70, cy - 10), (cx, cy + 70), "#2fa596")):
        path(s, poly([a, b, c]), col, 0.85, "#ffffff", 0.8)
    path(s, poly([(cx - 40, cy - 40), (cx, cy - 40), (cx - 20, cy - 10)]), "#ffffff", 0.7)
    path(s, poly([(cx, cy - 40), (cx + 40, cy - 40), (cx + 20, cy - 10)]), "#b4f1e8", 0.8)
    rng = random.Random(7)
    for i in range(26):
        x, y = rng.uniform(40, W - 40), rng.uniform(20, 380)
        path(s, poly(star_pts(x, y, 4, rng.uniform(4, 11), 1.2)), "#f4fffd", rng.uniform(0.3, 0.85))
    for r in range(30, 330, 26):
        ellipse(s, 640, 380, r * 2.2, r * 0.45, "none", max(0.05, 0.4 - r / 900), "#7fe0d3", 1.5)
    for k in range(6):
        path(s, wave_line(330 + k * 12, 3, 160, k), "none", 0.18, "#cfeee9", 1.5)
    return s


def crunch():
    s = Svg()
    sky(s, [(0, "#14070c", 1), (0.6, "#2c0d17", 1), (1, "#4a1e2a", 1)])
    # art-deco sunburst fan
    cx, cy = 705, 250
    for i in range(23):
        a = math.radians(180 + 4 + i * 8)
        line(s, cx, cy, cx + 700 * math.cos(a), cy + 700 * math.sin(a), "#e8c26a", 2 if i % 2 else 1, 0.22)
    for r in (120, 170, 220):
        path(s, f"M{cx - r} {cy} A{r} {r} 0 0 1 {cx + r} {cy}", "none", 0.25, "#e8c26a", 2)
    # martini glass
    gx, gy = 705, 30
    path(s, poly([(gx - 70, gy), (gx + 70, gy), (gx, gy + 70)]), s.lin([(0, "#f7e7b8", 0.9), (1, "#e8c26a", 0.5)]), None, "#fff3cf", 2)
    path(s, poly([(gx - 58, gy + 8), (gx + 58, gy + 8), (gx, gy + 62)]), "#b02a3e", 0.75)
    rect(s, gx - 3, gy + 68, 6, 50, "#f7e7b8")
    ellipse(s, gx, gy + 120, 36, 7, "#f7e7b8")
    line(s, gx + 20, gy - 30, gx - 18, gy + 40, "#c9a14a", 3)
    circle(s, gx + 2, gy + 12, 11, "#6f9a2a", None, "#2c3d10", 2)
    circle(s, gx + 4, gy + 10, 3, "#c0283b")
    # bubbles, a second glass and ice cubes
    rng = random.Random(41)
    for i in range(40):
        circle(s, rng.uniform(80, W - 80), rng.uniform(30, 380), rng.uniform(1.5, 6), "none", rng.uniform(0.2, 0.6), "#f7e7b8", 1.2)
    for x, y in ((150, 300), (1040, 290)):
        path(s, poly([(x - 30, y - 6), (x + 30, y - 6), (x + 26, y + 40), (x - 26, y + 40)]),
             s.lin([(0, "#e89a2a", 0.85), (1, "#8a3a0a", 0.95)]))
        rect(s, x - 20, y - 18, 20, 20, "#ffffff", 0.35, 4, "#ffffff", 1.5)
        rect(s, x + 2, y - 4, 18, 18, "#ffffff", 0.3, 4, "#ffffff", 1.5)
        path(s, poly([(x - 36, y - 60), (x + 36, y - 60), (x + 30, y + 46), (x - 30, y + 46)]), "#f7e7b8", 0.12, "#f7e7b8", 2.5)
        line(s, x - 26, y - 50, x - 22, y + 36, "#ffffff", 2, 0.45)
    # deco border
    for y in (8, 392):
        line(s, 30, y, W - 30, y, "#e8c26a", 2, 0.5)
    for x in range(60, W, 60):
        path(s, poly([(x, 392), (x + 8, 384), (x + 16, 392), (x + 8, 400)]), "#e8c26a", 0.5)
    return s


def eightbit():
    s = Svg()
    P = 8  # pixel size
    sky(s, [(0, "#0b0e24", 1), (0.6, "#1a2457", 1), (1, "#2c3f85", 1)])
    rng = random.Random(8)
    for i in range(90):
        x, y = rng.randrange(0, W // P) * P, rng.randrange(0, 30) * P
        rect(s, x, y, P // 2 if rng.random() < 0.7 else P, P // 2 if rng.random() < 0.7 else P, "#ffffff", rng.uniform(0.4, 1))
    # pixel moon
    mx, my = 1150, 150
    for yy in range(-4, 5):
        for xx in range(-4, 5):
            if xx * xx + yy * yy <= 18 and not ((xx - 2) ** 2 + (yy + 1) ** 2 <= 9):
                rect(s, mx + xx * P, my + yy * P, P, P, "#fff1a8")
    # pixel mountains
    for base_y, col, seed in ((300, "#20305f", 2), (330, "#152045", 5)):
        r = random.Random(seed)
        h = base_y
        for x in range(0, W, P):
            h += r.choice([-P, 0, 0, P])
            h = max(base_y - 110, min(base_y + 10, h))
            rect(s, x, h, P, H - h, col)
    # ground bricks
    for y in range(368, H, P * 2):
        rect(s, 0, y, W, P * 2, "#7a3a1c")
        off = 0 if (y // (P * 2)) % 2 == 0 else P * 2
        for x in range(-off, W, P * 4):
            rect(s, x, y, P * 4 - 2, P * 2 - 2, "#b5582a")
    # coins, hearts and a sword
    heart = ["01100110", "11111111", "11111111", "01111110", "00111100", "00011000"]
    coin = ["0011100", "0111110", "1101111", "1101111", "1101111", "0111110", "0011100"]
    for ox, oy, sprite, col in ((606, 34, heart, "#ff4d6d"), (668, 30, coin, "#ffd23f"), (718, 30, coin, "#ffd23f"),
                                (768, 30, coin, "#ffd23f"), (140, 220, heart, "#ff4d6d")):
        for yy, row in enumerate(sprite):
            for xx, c in enumerate(row):
                if c == "1":
                    rect(s, ox + xx * 6, oy + yy * 6, 6, 6, col)
    sword = ["000000011", "000000111", "000001110", "000011100", "100111000", "011110000", "001100000", "011010000", "110000000"]
    for yy, row in enumerate(sword):
        for xx, c in enumerate(row):
            if c == "1":
                rect(s, 1060 + xx * 6, 250 + yy * 6, 6, 6, "#cfe3ff" if xx + yy > 6 else "#8a5a2b")
    # pixel clouds
    for cx, cy in ((40, 118), (930, 112)):
        for dx, dy, w in ((0, 8, 12), (2, 0, 6), (6, 2, 5)):
            rect(s, cx + dx * P, cy + dy - 8, w * P, P * 2, "#e8f1ff", 0.85)
    return s


def castle(s, x, y, sc, fill):
    rect(s, x - 120 * sc, y - 70 * sc, 240 * sc, 70 * sc + 40, fill)
    for tx, th in ((-120, 130), (-40, 160), (40, 150), (120, 120)):
        rect(s, x + (tx - 20) * sc, y - th * sc, 40 * sc, th * sc, fill)
        for k in range(3):
            rect(s, x + (tx - 20 + k * 15) * sc, y - (th + 10) * sc, 9 * sc, 10 * sc, fill)
        path(s, poly([(x + tx * sc, y - (th + 10) * sc - 40 * sc), (x + tx * sc, y - (th + 10) * sc)]), "none", None, fill, 2)
        path(s, poly([(x + tx * sc, y - (th + 50) * sc), (x + (tx + 26) * sc, y - (th + 44) * sc), (x + tx * sc, y - (th + 36) * sc)]), "#d9b45a")
    path(s, f"M{f(x - 18 * sc)} {f(y + 40)} L{f(x - 18 * sc)} {f(y - 30 * sc)} A{f(18 * sc)} {f(18 * sc)} 0 0 1 {f(x + 18 * sc)} {f(y - 30 * sc)} L{f(x + 18 * sc)} {f(y + 40)} Z", "#d9b45a", 0.35)


def dragon(s, x, y, sc, fill, op=None):
    """Side view of a flying dragon, facing left; (x, y) is the shoulder."""
    def P(px, py):
        return f"{f(x + px * sc)} {f(y + py * sc)}"

    # far wing (slightly lighter so the two wings read apart)
    path(s, f"M{P(10, -4)} L{P(70, -70)} L{P(60, -118)} Q{P(80, -86)} {P(96, -80)} L{P(118, -112)} Q{P(118, -74)} {P(128, -64)} "
            f"L{P(160, -80)} Q{P(146, -46)} {P(146, -40)} L{P(178, -30)} Q{P(130, -12)} {P(96, 6)} Z", fill, 0.75 if op is None else op * 0.75)
    # body, neck, head and tail
    path(s, f"M{P(-20, 4)} C{P(-40, -10)} {P(-60, -34)} {P(-84, -40)} L{P(-104, -50)} L{P(-98, -40)} L{P(-128, -38)} "
            f"L{P(-112, -30)} L{P(-126, -22)} L{P(-96, -24)} C{P(-70, -16)} {P(-52, 2)} {P(-30, 18)} "
            f"C{P(10, 34)} {P(60, 34)} {P(100, 22)} C{P(150, 10)} {P(190, 30)} {P(230, 24)} "
            f"L{P(262, 6)} L{P(252, 26)} L{P(270, 36)} L{P(236, 34)} C{P(190, 44)} {P(150, 26)} {P(100, 36)} "
            f"C{P(60, 48)} {P(10, 40)} {P(-20, 4)} Z", fill, op)
    # horns and legs
    path(s, f"M{P(-92, -46)} L{P(-80, -66)} L{P(-86, -44)} Z", fill, op)
    path(s, f"M{P(-84, -42)} L{P(-66, -58)} L{P(-76, -40)} Z", fill, op)
    path(s, f"M{P(20, 34)} L{P(14, 56)} L{P(30, 60)} L{P(26, 36)} Z M{P(80, 34)} L{P(84, 58)} L{P(98, 60)} L{P(92, 32)} Z", fill, op)
    # near wing
    path(s, f"M{P(0, 0)} L{P(30, -76)} L{P(-14, -128)} Q{P(18, -96)} {P(36, -90)} L{P(56, -132)} Q{P(62, -92)} {P(74, -80)} "
            f"L{P(112, -104)} Q{P(100, -64)} {P(98, -56)} L{P(140, -48)} Q{P(96, -20)} {P(66, 10)} Z", fill, op)


def knight():
    s = Svg()
    sky(s, [(0, "#070a14", 1), (0.6, "#16203a", 1), (1, "#2c3a5e", 1)])
    starfield(s, 160, 19, 0, 260, "#f2e6c4", 1.4)
    glow(s, 712, 64, 200, "#f2e6c4", 0.3)
    circle(s, 712, 64, 54, "#f6edd2")
    for cx, cy, r in ((694, 48, 9), (736, 82, 7), (708, 90, 5)):
        circle(s, cx, cy, r, "#d9cfb3", 0.6)
    dragon(s, 712, 92, 0.5, "#060a16", 0.97)
    hills(s, 340, 16, 500, "#0d1426")
    castle(s, 200, 330, 0.95, "#0a0f1d")
    # crossed swords and a shield, bottom right
    cx, cy = 1080, 300
    for side in (-1, 1):
        x0, y0, x1, y1 = cx - side * 70, cy + 60, cx + side * 70, cy - 50
        line(s, x0, y0, x1, y1, "#cfd8e6", 6)
        line(s, x0, y0, x1, y1, "#ffffff", 1.5, 0.8)
        hx, hy = cx - side * 46, cy + 40
        line(s, hx - 14, hy - 10 * side, hx + 14, hy + 10 * side, "#d9b45a", 5)
        circle(s, x0 - side * 4, y0 + 4, 5, "#d9b45a")
    path(s, f"M{cx - 40} {cy - 40} L{cx + 40} {cy - 40} L{cx + 40} {cy} C{cx + 40} {cy + 30} {cx + 15} {cy + 48} {cx} {cy + 58} "
            f"C{cx - 15} {cy + 48} {cx - 40} {cy + 30} {cx - 40} {cy} Z", "#2a3550", None, "#d9b45a", 4)
    path(s, f"M{cx} {cy - 34} L{cx} {cy + 48} M{cx - 34} {cy - 6} L{cx + 34} {cy - 6}", "none", None, "#d9b45a", 5)
    return s


def heavymetal():
    s = Svg()
    sky(s, [(0, "#020305", 1), (0.6, "#0a0f18", 1), (1, "#141c2a", 1)])
    rng = random.Random(29)
    for i in range(30):
        glow(s, rng.uniform(-50, W + 50), rng.uniform(-40, 160), rng.uniform(80, 180), "#3a4a66", rng.uniform(0.2, 0.45))
    for x, seed, h, wdt in ((640, 1, 300, 40), (300, 2, 260, 34), (960, 3, 280, 38), (120, 4, 180, 26), (1120, 5, 200, 30)):
        pts = bolt(seed, x, -10, h, wdt)
        glow(s, x, 40, 140, "#5cc8ff", 0.3)
        path(s, poly(pts, False), "none", 0.25, "#5cc8ff", 14, 'stroke-linejoin="round"')
        path(s, poly(pts, False), "none", 0.6, "#9fe3ff", 6, 'stroke-linejoin="round"')
        path(s, poly(pts, False), "none", 1, "#ffffff", 2.2, 'stroke-linejoin="round"')
        for k in range(2):
            i0 = rng.randrange(1, len(pts) - 1)
            bx, by = pts[i0]
            branch = bolt(seed * 10 + k, bx, by, h * 0.35, wdt * 0.8)
            path(s, poly(branch, False), "none", 0.7, "#bfeeff", 1.5)
    for i in range(160):
        x, y = rng.uniform(0, W), rng.uniform(0, H)
        line(s, x, y, x - 6, y + 22, "#9fb8d6", 1, 0.25)
    # chrome horizon
    rect(s, 0, 340, W, 60, s.lin([(0, "#0a0f18", 1), (0.3, "#9fb8d6", 0.5), (0.35, "#0a0f18", 1), (1, "#05070b", 1)]))
    return s


def synthwave():
    s = Svg()
    sky(s, [(0, "#0d0420", 1), (0.5, "#3a0f5e", 1), (0.75, "#a3207a", 1), (1, "#ff6b4a", 1)])
    starfield(s, 120, 37, 0, 200, "#ffffff", 1.3)
    cx, cy, R = 640, 230, 150
    glow(s, cx, cy, 330, "#ff3fa4", 0.45)
    circle(s, cx, cy, R, s.lin([(0, "#ffe66b", 1), (0.6, "#ff7a3a", 1), (1, "#ff2f8f", 1)]))
    for k in range(7):
        y = cy + 10 + k * 18
        rect(s, cx - R - 5, y, R * 2 + 10, 3 + k * 1.6, "#3a0f5e")
    # wireframe mountains
    for pts, col in (([(0, 300), (140, 210), (260, 290), (380, 240), (520, 300)], "#2ee6ff"),
                     ([(700, 300), (820, 230), (930, 280), (1060, 200), (1200, 300)], "#2ee6ff")):
        path(s, poly(pts + [(pts[-1][0], 300), (pts[0][0], 300)]), "#1a0733", 0.95, col, 2)
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            line(s, (x0 + x1) / 2, min(y0, y1), (x0 + x1) / 2, 300, col, 1, 0.4)
    # perspective grid
    rect(s, 0, 300, W, 100, "#12052a")
    for k in range(-14, 15):
        line(s, 640 + k * 30, 300, 640 + k * 200, 420, "#ff3fa4", 1.4, 0.75)
    y, gap = 302, 4
    while y < H:
        line(s, 0, y, W, y, "#ff3fa4", 1.4, 0.75)
        gap *= 1.35
        y += gap
    for x, fl in ((90, 1), (1110, -1)):
        palm(s, x, 320, 200, "#0d0420", 0.95, 0.15, fl)
    return s


def western():
    s = Svg()
    sky(s, [(0, "#3a1408", 1), (0.5, "#a3401a", 1), (0.8, "#f08a3a", 1), (1, "#ffd08a", 1)])
    glow(s, 640, 290, 380, "#ffd08a", 0.5)
    circle(s, 640, 290, 120, "#ffe0a0")
    for k in range(4):
        rect(s, 500, 300 + k * 16, 280, 5 + k, "#f08a3a", 0.8)
    for i in range(6):
        x, y = 300 + i * 120 + (i % 2) * 30, 60 + (i % 3) * 18
        path(s, f"M{x - 10} {y} Q{x - 5} {y - 5} {x} {y} Q{x + 5} {y - 5} {x + 10} {y}", "none", 0.8, "#2a0e05", 2)
    # mesas
    for pts in (([(0, 400), (0, 240), (40, 230), (60, 200), (260, 200), (280, 230), (330, 250), (360, 400)]),
                ([(860, 400), (900, 260), (930, 230), (1100, 230), (1130, 250), (1200, 260), (1200, 400)])):
        path(s, poly(pts), "#5a2210")
        path(s, poly(pts), "none", 0.35, "#ffb070", 2)
    hills(s, 350, 8, 400, "#3a1508")
    # saguaro cacti
    for x, h, sc in ((380, 170, 1.0), (1000, 140, 0.85), (120, 120, 0.7)):
        w = 18 * sc
        rect(s, x - w / 2, 400 - h, w, h, "#1f2a10", None, w / 2)
        for side, ay, ah in ((-1, 0.45, 0.35), (1, 0.6, 0.3)):
            ax = x + side * 28 * sc
            rect(s, min(x, ax) + (0 if side > 0 else 0), 400 - h * ay, abs(ax - x), w * 0.8, "#1f2a10", None, w * 0.4)
            rect(s, ax - w * 0.4, 400 - h * (ay + ah), w * 0.8, h * ah + w * 0.6, "#1f2a10", None, w * 0.4)
    # sheriff star, top centre
    cx, cy = 705, 64
    path(s, poly(star_pts(cx, cy, 5, 48, 22)), "#e0b04a", None, "#7a4a12", 3)
    circle(s, cx, cy, 18, "none", None, "#7a4a12", 2)
    for k in range(5):
        a = math.radians(-90 + k * 72)
        circle(s, cx + 48 * math.cos(a), cy + 48 * math.sin(a), 6, "#e0b04a", None, "#7a4a12", 2)
    rng = random.Random(4)
    for i in range(80):
        circle(s, rng.uniform(0, W), rng.uniform(340, 400), rng.uniform(0.6, 1.8), "#ffcf9a", 0.4)
    return s


def funk():
    s = Svg()
    sky(s, [(0, "#14061c", 1), (1, "#3e1650", 1)])
    cx, cy = 705, 74
    for i in range(30):
        a = math.radians(i * 12)
        col = ["#ffc23a", "#ff5ab4", "#5ad8ff"][i % 3]
        path(s, poly([(cx, cy), (cx + 900 * math.cos(a - 0.03), cy + 900 * math.sin(a - 0.03)),
                      (cx + 900 * math.cos(a + 0.03), cy + 900 * math.sin(a + 0.03))]), col, 0.14)
    # 70s stripes
    for k, col in enumerate(("#ff8a1f", "#ffc23a", "#e8442e", "#8a3a1a")):
        path(s, wave_line(330 + k * 14, 18, 520, 0.8), "none", 0.9, col, 12)
    # disco ball
    R = 54
    line(s, cx, -10, cx, cy - R, "#d8d8d8", 2)
    glow(s, cx, cy, 170, "#ffffff", 0.35)
    circle(s, cx, cy, R, s.rad([(0, "#ffffff", 1), (0.6, "#b9c2cc", 1), (1, "#4a4f57", 1)], 0.35, 0.35, 0.8))
    rng = random.Random(5)
    for row in range(-5, 6):
        y = cy + row * (R / 5.5)
        half = math.sqrt(max(0.0, R * R - (y - cy) ** 2))
        n = max(2, int(half / 7))
        for k in range(-n, n):
            x = cx + k * (half / n)
            rect(s, x, y - 4.5, half / n - 1.5, R / 5.5 - 1.5, rng.choice(["#ffffff", "#dfe6ee", "#9aa4b0", "#ffe9a8", "#bfe8ff"]),
                 rng.uniform(0.45, 0.95))
    for i in range(40):
        x, y = rng.uniform(40, W - 40), rng.uniform(20, 300)
        path(s, poly(star_pts(x, y, 4, rng.uniform(4, 10), 1.2)), rng.choice(["#ffffff", "#ffc23a", "#ff9ad6"]), rng.uniform(0.4, 0.9))
    return s


def pharaoh():
    s = Svg()
    sky(s, [(0, "#0e1530", 1), (0.5, "#4a2a4a", 1), (0.8, "#d27a3a", 1), (1, "#f2c06a", 1)])
    starfield(s, 90, 43, 0, 150, "#fff3cf", 1.2)
    # sun disc with rays
    cx, cy = 705, 70
    for i in range(17):
        a = math.radians(30 + i * 7.5)
        x1, y1 = cx + 60 * math.cos(a), cy + 60 * math.sin(a)
        x2, y2 = cx + 300 * math.cos(a), cy + 300 * math.sin(a)
        line(s, x1, y1, x2, y2, "#ffd36a", 3, 0.35)
        circle(s, x2, y2, 4, "#ffd36a", 0.5)
    glow(s, cx, cy, 130, "#ffd36a", 0.5)
    circle(s, cx, cy, 46, s.lin([(0, "#fff0b8", 1), (1, "#e2a23a", 1)]), None, "#1f4fa8", 3)
    # eye
    ex, ey = cx, cy
    path(s, f"M{ex - 26} {ey} Q{ex} {ey - 20} {ex + 26} {ey} Q{ex} {ey + 18} {ex - 26} {ey} Z", "#fff6dc", None, "#1b2a55", 3)
    circle(s, ex, ey, 8, "#1b2a55")
    path(s, f"M{ex - 8} {ey + 12} L{ex - 14} {ey + 30} M{ex + 10} {ey + 12} Q{ex + 26} {ey + 26} {ex + 32} {ey + 14}", "none", None, "#1b2a55", 3)
    # pyramids
    for px, py, w, h in ((220, 380, 300, 190), (430, 380, 200, 120), (980, 380, 320, 200)):
        path(s, poly([(px - w / 2, py), (px, py - h), (px + w / 2, py)]), "#b8853a")
        path(s, poly([(px, py - h), (px + w / 2, py), (px + w * 0.12, py)]), "#7a5320")
        for k in range(1, 8):
            yy = py - h + h * k / 8
            line(s, px - w / 2 * k / 8, yy, px + w / 2 * k / 8, yy, "#5a3a14", 1, 0.35)
    hills(s, 372, 10, 300, "#d9a85a")
    hills(s, 388, 6, 200, "#c08a40", None, 1.4)
    for x, fl in ((680, 1), (760, -1)):
        palm(s, x, 390, 110, "#2a1a0a", 0.9, 0.2, fl)
    # lotus border along the very bottom
    for x in range(20, W, 46):
        path(s, f"M{x} 400 Q{x + 6} 384 {x + 12} 400 Q{x + 18} 380 {x + 24} 400 Q{x + 30} 384 {x + 36} 400 Z", "#1f4fa8", 0.8)
    return s


def blossom(s, x, y, r, rng):
    for k in range(5):
        a = math.radians(k * 72 + rng.uniform(0, 72))
        ellipse(s, x + r * 0.55 * math.cos(a), y + r * 0.55 * math.sin(a), r * 0.55, r * 0.38, "#f7b8c8", 0.95, rot=math.degrees(a))
    circle(s, x, y, r * 0.22, "#c8102e")


def sakura():
    s = Svg()
    sky(s, [(0, "#2a1220", 1), (0.5, "#7a3a55", 1), (0.85, "#e8a0b0", 1), (1, "#f6d6dc", 1)])
    glow(s, 640, 170, 220, "#ff6b6b", 0.35)
    circle(s, 640, 170, 104, "#d6243a")
    # Mt Fuji
    path(s, "M260 400 L560 190 Q640 150 720 190 L1020 400 Z", "#3b2a4a")
    path(s, "M560 190 Q640 150 720 190 L690 215 L665 200 L640 222 L615 200 L590 215 Z", "#ffffff", 0.95)
    # seigaiha waves at the bottom
    for row in range(4):
        for col in range(-1, 26):
            x = col * 52 + (26 if row % 2 else 0)
            y = 340 + row * 18
            for r, c in ((26, "#22356b"), (19, "#f6f1e7"), (13, "#22356b"), (7, "#f6f1e7")):
                path(s, f"M{x - r} {y + 26} A{r} {r} 0 0 1 {x + r} {y + 26} Z", c)
    # blossom branches from the top corners
    rng = random.Random(15)
    path(s, "M600 -6 C640 30 690 20 760 50 C790 62 805 80 812 100", "none", None, "#3a1d14", 8)
    path(s, "M668 22 C672 50 660 70 640 92", "none", None, "#3a1d14", 5)
    path(s, "M735 40 C750 20 770 12 790 10", "none", None, "#3a1d14", 4)
    for i in range(30):
        t = rng.uniform(0, 1)
        bx = 610 + 200 * t + rng.uniform(-14, 14)
        by = 10 + 70 * t + rng.uniform(-14, 24)
        blossom(s, bx, by, rng.uniform(8, 13), rng)
    for bx, by in ((1150, 140), (1120, 175), (60, 150), (90, 185)):
        blossom(s, bx, by, 12, rng)
    for i in range(50):
        x, y = rng.uniform(0, W), rng.uniform(40, 330)
        ellipse(s, x, y, rng.uniform(3, 6), rng.uniform(2, 3), "#f7b8c8", rng.uniform(0.5, 0.95), rot=rng.uniform(0, 180))
    # torii gate silhouette
    tx = 1000
    rect(s, tx - 70, 230, 140, 10, "#c8102e"); rect(s, tx - 80, 220, 160, 12, "#c8102e", None, 4)
    rect(s, tx - 50, 240, 10, 110, "#c8102e"); rect(s, tx + 40, 240, 10, 110, "#c8102e"); rect(s, tx - 60, 255, 120, 8, "#c8102e")
    return s


def girih(s, x0, y0, w, h, cell, col, op):
    for y in range(int(y0), int(y0 + h + cell), int(cell)):
        for x in range(int(x0), int(x0 + w + cell), int(cell)):
            path(s, poly(star_pts(x, y, 8, cell * 0.42, cell * 0.24, -90 + 22.5)), "none", op, col, 1.4)
            path(s, poly([(x + cell / 2, y - cell * 0.18), (x + cell / 2 + cell * 0.18, y), (x + cell / 2, y + cell * 0.18),
                          (x + cell / 2 - cell * 0.18, y)]), "none", op, col, 1.2)


def persian():
    s = Svg()
    sky(s, [(0, "#06111f", 1), (0.6, "#0b2a45", 1), (1, "#12507a", 1)])
    girih(s, 0, 0, W, H, 64, "#e6c068", 0.22)
    # pointed arch (iwan) framing the top centre
    cx = 705
    for k, (r, col, sw, op) in enumerate(((150, "#e6c068", 6, 0.9), (132, "#3fc1c9", 3, 0.8), (118, "#e6c068", 2, 0.7))):
        path(s, f"M{cx - r} 400 L{cx - r} 140 Q{cx - r} {20 + k * 14} {cx} {-20 + k * 16} Q{cx + r} {20 + k * 14} {cx + r} 140 L{cx + r} 400",
             "none", op, col, sw)
    # rosette in the arch
    for k in range(12):
        a = math.radians(k * 30)
        ellipse(s, cx + 28 * math.cos(a), 70 + 28 * math.sin(a), 22, 9, "#3fc1c9", 0.7, rot=k * 30)
    circle(s, cx, 70, 14, "#e6c068")
    path(s, poly(star_pts(cx, 70, 8, 10, 5)), "#12507a")
    # tile borders
    for y in (0, 384):
        rect(s, 0, y, W, 16, "#0b3352")
        for x in range(0, W, 32):
            path(s, poly(star_pts(x + 16, y + 8, 4, 7, 3, -90 + 45)), "#3fc1c9", 0.8)
    rng = random.Random(9)
    for i in range(50):
        circle(s, rng.uniform(0, W), rng.uniform(20, 380), rng.uniform(0.6, 1.8), "#ffe9b0", rng.uniform(0.3, 0.8))
    return s


def rose_window(s, cx, cy, R):
    cols = ["#c0283b", "#2a55c8", "#e0a82e", "#2f8f5a", "#7b3fb5"]
    circle(s, cx, cy, R + 8, "#1c1a20")
    for ring, (r0, r1, n) in enumerate(((R * 0.55, R, 16), (R * 0.25, R * 0.55, 8))):
        for k in range(n):
            a0, a1 = 2 * math.pi * k / n, 2 * math.pi * (k + 1) / n
            pts = [(cx + r0 * math.cos(a0), cy + r0 * math.sin(a0)), (cx + r1 * math.cos(a0), cy + r1 * math.sin(a0)),
                   (cx + r1 * math.cos((a0 + a1) / 2) * 1.0, cy + r1 * math.sin((a0 + a1) / 2)),
                   (cx + r1 * math.cos(a1), cy + r1 * math.sin(a1)), (cx + r0 * math.cos(a1), cy + r0 * math.sin(a1))]
            path(s, poly(pts), cols[(k + ring) % 5], 0.95, "#1c1a20", 2.5)
    circle(s, cx, cy, R * 0.25, "#e0a82e", None, "#1c1a20", 3)
    path(s, poly(star_pts(cx, cy, 6, R * 0.2, R * 0.1)), "#fff1c4")


def choir():
    s = Svg()
    sky(s, [(0, "#0d0b16", 1), (0.6, "#1f1b30", 1), (1, "#2c2740", 1)])
    # light beams from the rose window
    cx, cy = 705, 66
    for k, (dx, a) in enumerate(((-220, 0.10), (-60, 0.14), (120, 0.12), (300, 0.08))):
        path(s, poly([(cx - 30 + k * 20, cy), (cx + 30 + k * 20, cy), (cx + dx + 120, 420), (cx + dx - 60, 420)]),
             s.lin([(0, "#ffe9a8", a * 3), (1, "#ffe9a8", 0)]))
    glow(s, cx, cy, 150, "#ffe9a8", 0.35)
    rose_window(s, cx, cy, 58)
    # gothic arches and columns
    for x in (90, 250, 1070, 1230):
        rect(s, x - 14, 120, 28, 300, "#151225")
        path(s, f"M{x - 80} 420 L{x - 80} 170 Q{x - 80} 70 {x} 30 Q{x + 80} 70 {x + 80} 170 L{x + 80} 420", "none", 0.9, "#3a3456", 10)
        path(s, f"M{x - 50} 420 L{x - 50} 180 Q{x - 50} 110 {x} 80 Q{x + 50} 110 {x + 50} 180 L{x + 50} 420", "none", 0.8, "#4a4370", 4)
        # stained glass lancet
        path(s, f"M{x - 36} 300 L{x - 36} 185 Q{x - 36} 130 {x} 108 Q{x + 36} 130 {x + 36} 185 L{x + 36} 300 Z",
             s.lin([(0, "#2a55c8", 0.7), (0.5, "#7b3fb5", 0.6), (1, "#c0283b", 0.6)]))
        for yy in range(130, 300, 26):
            line(s, x - 36, yy, x + 36, yy, "#151225", 2)
        line(s, x, 108, x, 300, "#151225", 2)
    # candles
    rng = random.Random(3)
    for i, x in enumerate(range(380, 960, 46)):
        h = 30 + (i % 3) * 14
        rect(s, x - 6, 400 - h, 12, h, "#f2ead6")
        glow(s, x, 400 - h - 10, 26, "#ffd36a", 0.7)
        path(s, flame_path(x, 400 - h - 2, 8, 20), "#ffe9a8")
    for i in range(30):
        circle(s, rng.uniform(300, 1000), rng.uniform(140, 380), rng.uniform(0.8, 2), "#fff1c4", rng.uniform(0.3, 0.8))
    return s


SCENES = {
    "dream": dream, "dub": dub, "spacedelay": spacedelay, "blackmetal": blackmetal, "warmrock": warmrock,
    "surf": surf, "shoegaze": shoegaze, "arena": arena, "stereojazz": stereojazz, "doom": doom,
    "crunch": crunch, "eightbit": eightbit, "knight": knight, "heavymetal": heavymetal, "synthwave": synthwave,
    "western": western, "funk": funk, "pharaoh": pharaoh, "sakura": sakura, "persian": persian, "choir": choir,
    "drive": drive, "clean": clean,
}


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, scene in SCENES.items():
        svg = scene().render()
        with open(os.path.join(OUT, name + ".svg"), "w") as fh:
            fh.write(svg)
        print(f"{name:12s} {len(svg) // 1024:4d} KB")
