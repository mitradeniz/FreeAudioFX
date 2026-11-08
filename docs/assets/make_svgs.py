"""Generates the animated SVG artwork for the README (docs/assets/*.svg).

Usage:  python3 docs/assets/make_svgs.py docs/assets

All animation is CSS (transform / opacity / stroke-dashoffset), which GitHub
renders when an SVG is embedded with <img>. Motion is disabled for viewers who
ask for reduced motion.
"""
import math, os, sys

OUT = sys.argv[1] if len(sys.argv) > 1 else "."
FONT = "'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif"
MONO = "SFMono-Regular, Menlo, Consolas, 'Liberation Mono', monospace"
ORANGE, TEAL = "#ff5a1f", "#19b3a3"

REDUCED = "@media (prefers-reduced-motion: reduce) { * { animation: none !important; } }"


def wave_path(x0, x1, yc, amp, period, shape):
    pts = []
    x = x0
    while x <= x1 + 0.01:
        ph = 2 * math.pi * (x - x0) / period
        s = math.sin(ph)
        if shape == "clean":
            y = s
        else:  # hard-ish tube clipping, slightly asymmetric
            y = math.tanh(5.0 * s + 0.6) / math.tanh(5.6) - 0.08
        pts.append((x, yc - amp * y))
        x += 2.0
    d = "M" + " L".join(f"{px:.1f},{py:.1f}" for px, py in pts)
    return d


# ---------------------------------------------------------------------------
def mini_amp(x, y, w, h, name, accent, plate_top, plate_bot, text_col, knob_col, skirt_col, idx):
    """A tiny amp head with wiggling knobs and a pulsing jewel."""
    g = []
    g.append(f'<g transform="translate({x},{y})">')
    # handle
    g.append(f'<rect x="{w/2-55}" y="-14" width="110" height="20" rx="10" fill="none" stroke="#2b2b2f" stroke-width="7"/>')
    # cabinet
    g.append(f'<rect width="{w}" height="{h}" rx="14" fill="url(#tolex)" stroke="#000" stroke-opacity=".6"/>')
    # faceplate
    g.append(f'<linearGradient id="plate{idx}" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{plate_top}"/><stop offset="1" stop-color="{plate_bot}"/></linearGradient>')
    g.append(f'<rect x="14" y="40" width="{w-28}" height="{h-54}" rx="7" fill="url(#plate{idx})" stroke="#000" stroke-opacity=".35"/>')
    # logo
    first, rest = name.split(" ")
    g.append(f'<text x="22" y="29" font-family="{FONT}" font-weight="800" font-size="19" letter-spacing="1.5" fill="{accent}">{first}<tspan fill="#f2f2f2"> {rest}</tspan></text>')
    # jewel
    g.append(f'<circle cx="{w-30}" cy="22" r="11" fill="{accent}" opacity=".35" class="glow"/>')
    g.append(f'<circle cx="{w-30}" cy="22" r="6.5" fill="{accent}" stroke="#000" stroke-width="2"/>')
    # knobs
    n = 6
    span = w - 70
    for k in range(n):
        cx = 38 + k * span / (n - 1)
        cy = 40 + (h - 54) / 2 + 2
        delay = (k * 0.37 + idx * 0.9) % 3
        dur = 2.6 + (k % 3) * 0.5
        g.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="15" fill="{skirt_col}" stroke="#000" stroke-opacity=".5"/>')
        g.append(f'<g class="knob" style="animation-delay:-{delay:.2f}s;animation-duration:{dur:.1f}s">'
                 f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="11" fill="{knob_col}"/>'
                 f'<line x1="{cx:.1f}" y1="{cy-2:.1f}" x2="{cx:.1f}" y2="{cy-10:.1f}" stroke="#f4f4f4" stroke-width="2.4" stroke-linecap="round"/></g>')
    g.append('</g>')
    return "\n".join(g)


def hero():
    W, H = 1200, 430
    clean = wave_path(0, 1200 + 160, 0, 26, 80, "clean")
    dirty = wave_path(0, 1200 + 160, 0, 26, 80, "dirty")
    s = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" aria-label="Open Audio Plugins - free amp simulators and audio effects">
<title>Open Audio Plugins</title>
<style>
  .knob {{ transform-box: fill-box; transform-origin: center; animation: wiggle 3s ease-in-out infinite alternate; }}
  @keyframes wiggle {{ 0% {{ transform: rotate(-120deg); }} 100% {{ transform: rotate(110deg); }} }}
  .glow {{ transform-box: fill-box; transform-origin: center; animation: pulse 1.6s ease-in-out infinite; }}
  @keyframes pulse {{ 0%,100% {{ opacity: .15; transform: scale(.9); }} 50% {{ opacity: .6; transform: scale(1.35); }} }}
  .scroll {{ animation: scroll 1.6s linear infinite; }}
  @keyframes scroll {{ from {{ transform: translateX(0); }} to {{ transform: translateX(-80px); }} }}
  .bar {{ transform-box: fill-box; transform-origin: bottom; animation: eq 1.1s ease-in-out infinite alternate; }}
  @keyframes eq {{ 0% {{ transform: scaleY(.25); }} 100% {{ transform: scaleY(1); }} }}
  .float {{ animation: float 4s ease-in-out infinite; }}
  @keyframes float {{ 0%,100% {{ transform: translateY(0); }} 50% {{ transform: translateY(-6px); }} }}
  {REDUCED}
</style>
<defs>
  <linearGradient id="bg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#17171b"/><stop offset="1" stop-color="#0e0e10"/></linearGradient>
  <pattern id="hatch" width="6" height="6" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><line x1="0" y1="0" x2="0" y2="6" stroke="#fff" stroke-opacity=".025" stroke-width="2"/></pattern>
  <linearGradient id="tolex" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#232327"/><stop offset="1" stop-color="#151518"/></linearGradient>
  <linearGradient id="title" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="{ORANGE}"/><stop offset="1" stop-color="#ffb347"/></linearGradient>
  <linearGradient id="fadeL" x1="0" x2="1"><stop offset="0" stop-color="#fff" stop-opacity="0"/><stop offset=".12" stop-color="#fff"/><stop offset=".88" stop-color="#fff"/><stop offset="1" stop-color="#fff" stop-opacity="0"/></linearGradient>
  <mask id="fade"><rect x="0" y="330" width="{W}" height="90" fill="url(#fadeL)"/></mask>
  <clipPath id="leftHalf"><rect x="0" y="320" width="560" height="110"/></clipPath>
  <clipPath id="rightHalf"><rect x="640" y="320" width="560" height="110"/></clipPath>
  <radialGradient id="ampGlow"><stop offset="0" stop-color="{ORANGE}" stop-opacity=".55"/><stop offset="1" stop-color="{ORANGE}" stop-opacity="0"/></radialGradient>
</defs>

<rect width="{W}" height="{H}" rx="26" fill="url(#bg)"/>
<rect width="{W}" height="{H}" rx="26" fill="url(#hatch)"/>
<rect x=".5" y=".5" width="{W-1}" height="{H-1}" rx="26" fill="none" stroke="#fff" stroke-opacity=".07"/>

<!-- title -->
<text x="64" y="118" font-family="{FONT}" font-weight="800" font-size="70" fill="#f5f5f5">Open Audio</text>
<text x="64" y="192" font-family="{FONT}" font-weight="800" font-size="70" fill="url(#title)">Plugins</text>
<text x="66" y="238" font-family="{FONT}" font-size="23" fill="#a9abb2">33 free amps &amp; pedals for your DAW</text>

<!-- chips -->
<g font-family="{FONT}" font-size="15" font-weight="700">
  <rect x="64" y="262" width="76" height="30" rx="15" fill="#26262b" stroke="#3a3a40"/><text x="102" y="282" text-anchor="middle" fill="#e8e8e8">VST3</text>
  <rect x="150" y="262" width="56" height="30" rx="15" fill="#26262b" stroke="#3a3a40"/><text x="178" y="282" text-anchor="middle" fill="#e8e8e8">AU</text>
  <rect x="216" y="262" width="124" height="30" rx="15" fill="#26262b" stroke="#3a3a40"/><text x="278" y="282" text-anchor="middle" fill="#e8e8e8">Standalone</text>
  <rect x="350" y="262" width="150" height="30" rx="15" fill="{ORANGE}" fill-opacity=".15" stroke="{ORANGE}" stroke-opacity=".6"/><text x="425" y="282" text-anchor="middle" fill="#ffb08f">100% free</text>
</g>

<!-- amps -->
<g class="float">
{mini_amp(650, 46, 480, 118, "DRIVE AMP", ORANGE, "#3a3c41", "#202125", "#eee", "#17171a", "#5f636b", 0)}
</g>
<g class="float" style="animation-delay:-2s">
{mini_amp(650, 196, 480, 118, "CLEAN AMP", TEAL, "#efe9dd", "#cfc8b8", "#222", "#151515", "#c4c8cd", 1)}
</g>

<!-- signal: clean in, distorted out -->
<g mask="url(#fade)">
  <g clip-path="url(#leftHalf)"><g transform="translate(0,372)"><path class="scroll" d="{clean}" fill="none" stroke="{TEAL}" stroke-width="3" stroke-linejoin="round"/></g></g>
  <g clip-path="url(#rightHalf)"><g transform="translate(0,372)"><path class="scroll" d="{dirty}" fill="none" stroke="{ORANGE}" stroke-width="3" stroke-linejoin="round"/></g></g>
</g>
<circle cx="600" cy="372" r="58" fill="url(#ampGlow)" class="glow"/>
<circle cx="600" cy="372" r="30" fill="#1b1b1f" stroke="{ORANGE}" stroke-width="2.5"/>
<path d="M604 352 L590 375 L601 375 L596 393 L611 368 L600 368 Z" fill="{ORANGE}"/>
<text x="64" y="408" font-family="{FONT}" font-size="13" font-weight="700" letter-spacing="2" fill="{TEAL}" opacity=".85">CLEAN IN</text>
<text x="1136" y="408" text-anchor="end" font-family="{FONT}" font-size="13" font-weight="700" letter-spacing="2" fill="{ORANGE}" opacity=".9">DRIVE OUT</text>
</svg>
'''
    return s


# ---------------------------------------------------------------------------
def signal_flow():
    W, H = 1200, 330
    xs = [120, 360, 600, 840, 1080]
    cy = 135
    card = 170

    def card_rect(x, accent):
        return (f'<rect x="{x-card/2}" y="{cy-card/2}" width="{card}" height="{card}" rx="26" fill="#1c1c20" stroke="{accent}" stroke-opacity=".55" stroke-width="2"/>')

    guitar = f'''<g transform="translate({xs[0]-10},{cy+32}) rotate(32) scale(.66)"><g class="strum">
      <rect x="-10" y="-150" width="20" height="30" rx="6" fill="#2a2a2e"/>
      <g fill="#d8d8d8"><circle cx="-14" cy="-144" r="3.5"/><circle cx="-14" cy="-132" r="3.5"/><circle cx="14" cy="-144" r="3.5"/><circle cx="14" cy="-132" r="3.5"/></g>
      <rect x="-7" y="-122" width="14" height="110" rx="2" fill="#6b4226"/>
      <g stroke="#c9c9c9" stroke-width="1.2"><line x1="-7" y1="-100" x2="7" y2="-100"/><line x1="-7" y1="-80" x2="7" y2="-80"/><line x1="-7" y1="-62" x2="7" y2="-62"/><line x1="-7" y1="-46" x2="7" y2="-46"/><line x1="-7" y1="-32" x2="7" y2="-32"/></g>
      <path d="M-9,-16 C-14,-30 -22,-46 -31,-42 C-41,-37 -35,-16 -39,-2 C-45,14 -47,40 -25,51 C-10,58 10,58 25,51 C47,40 43,14 37,0 C33,-10 39,-27 31,-34 C25,-38 16,-25 9,-16 Z" fill="{ORANGE}" stroke="#000" stroke-opacity=".45" stroke-width="2.5"/>
      <path d="M-9,-14 C-20,-12 -31,6 -29,24 C-27,39 -10,45 6,41 L8,-14 Z" fill="#f3efe6" opacity=".9"/>
      <g fill="#222"><rect x="-13" y="-4" width="26" height="7" rx="2"/><rect x="-13" y="12" width="26" height="7" rx="2"/></g>
      <rect x="-14" y="30" width="28" height="7" rx="2" fill="#c9c9c9"/>
      <g stroke="#f4f4f4" stroke-width="1"><line x1="-4" y1="-128" x2="-4" y2="34"/><line x1="0" y1="-128" x2="0" y2="34"/><line x1="4" y1="-128" x2="4" y2="34"/></g>
      <g fill="#eee" stroke="#000" stroke-opacity=".4"><circle cx="22" cy="24" r="5"/><circle cx="27" cy="37" r="5"/></g>
    </g></g>'''

    interface = f'''<g transform="translate({xs[1]},{cy})">
      <rect x="-58" y="-32" width="116" height="64" rx="10" fill="#c23b3b" stroke="#000" stroke-opacity=".5" stroke-width="2"/>
      <rect x="-50" y="-24" width="100" height="48" rx="6" fill="#a42e2e"/>
      <circle cx="-30" cy="-2" r="11" fill="#1d1d1d"/><circle cx="-30" cy="-2" r="5" fill="#666"/>
      <circle cx="0" cy="-2" r="11" fill="#1d1d1d"/><circle cx="0" cy="-2" r="5" fill="#666"/>
      <g class="ledrow"><rect x="20" y="-14" width="22" height="5" rx="2" fill="#4cd964"/><rect x="20" y="-6" width="22" height="5" rx="2" fill="#4cd964"/><rect x="20" y="2" width="22" height="5" rx="2" fill="#ffcc00" class="blink"/></g>
      <circle cx="-30" cy="17" r="3" fill="#ddd"/><circle cx="0" cy="17" r="3" fill="#ddd"/>
    </g>'''

    laptop = f'''<g transform="translate({xs[2]},{cy})">
      <rect x="-56" y="-46" width="112" height="72" rx="7" fill="#2b2b30" stroke="#000" stroke-opacity=".5" stroke-width="2"/>
      <rect x="-49" y="-39" width="98" height="58" rx="3" fill="#3a3a40"/>
      <g class="clips">
        <rect x="-44" y="-34" width="18" height="8" rx="2" fill="#7b68ee"/><rect x="-22" y="-34" width="18" height="8" rx="2" fill="#e05cc5"/><rect x="0" y="-34" width="18" height="8" rx="2" fill="#b58cf0"/><rect x="22" y="-34" width="18" height="8" rx="2" fill="#e85d8a"/>
        <rect x="-44" y="-22" width="18" height="8" rx="2" fill="#555"/><rect x="-22" y="-22" width="18" height="8" rx="2" fill="#555"/><rect x="0" y="-22" width="18" height="8" rx="2" fill="#ffa64d" class="blink"/><rect x="22" y="-22" width="18" height="8" rx="2" fill="#555"/>
      </g>
      <g class="eqbars"><rect class="bar" x="-40" y="-6" width="6" height="20" fill="{TEAL}"/><rect class="bar b2" x="-30" y="-6" width="6" height="20" fill="{TEAL}"/><rect class="bar b3" x="-20" y="-6" width="6" height="20" fill="{TEAL}"/><rect class="bar b4" x="-10" y="-6" width="6" height="20" fill="{ORANGE}"/><rect class="bar b5" x="0" y="-6" width="6" height="20" fill="{ORANGE}"/></g>
      <path d="M-70,28 L70,28 L62,38 L-62,38 Z" fill="#45454c" stroke="#000" stroke-opacity=".5"/>
    </g>'''

    amp = f'''<g transform="translate({xs[3]},{cy})">
      <rect x="-26" y="-50" width="52" height="14" rx="7" fill="none" stroke="#333" stroke-width="5"/>
      <rect x="-62" y="-42" width="124" height="78" rx="10" fill="#202024" stroke="#000" stroke-opacity=".6" stroke-width="2"/>
      <rect x="-54" y="-12" width="108" height="40" rx="5" fill="#33353a"/>
      <text x="-52" y="-20" font-family="{FONT}" font-weight="800" font-size="12" fill="{ORANGE}">DRIVE<tspan fill="#eee"> AMP</tspan></text>
      <circle cx="46" cy="-25" r="9" fill="{ORANGE}" class="glow" opacity=".4"/><circle cx="46" cy="-25" r="5" fill="{ORANGE}"/>
      <g><circle cx="-36" cy="8" r="9" fill="#5f636b"/><g class="knob"><circle cx="-36" cy="8" r="6.5" fill="#111"/><line x1="-36" y1="7" x2="-36" y2="1" stroke="#fff" stroke-width="2" stroke-linecap="round"/></g></g>
      <g><circle cx="-12" cy="8" r="9" fill="#5f636b"/><g class="knob" style="animation-delay:-.7s"><circle cx="-12" cy="8" r="6.5" fill="#111"/><line x1="-12" y1="7" x2="-12" y2="1" stroke="#fff" stroke-width="2" stroke-linecap="round"/></g></g>
      <g><circle cx="12" cy="8" r="9" fill="#5f636b"/><g class="knob" style="animation-delay:-1.3s"><circle cx="12" cy="8" r="6.5" fill="#111"/><line x1="12" y1="7" x2="12" y2="1" stroke="#fff" stroke-width="2" stroke-linecap="round"/></g></g>
      <g><circle cx="36" cy="8" r="9" fill="#5f636b"/><g class="knob" style="animation-delay:-2.1s"><circle cx="36" cy="8" r="6.5" fill="#111"/><line x1="36" y1="7" x2="36" y2="1" stroke="#fff" stroke-width="2" stroke-linecap="round"/></g></g>
    </g>'''

    phones = f'''<g transform="translate({xs[4]},{cy})">
      <path d="M-38,10 C-38,-48 38,-48 38,10" fill="none" stroke="#e8e8e8" stroke-width="8" stroke-linecap="round"/>
      <rect x="-50" y="0" width="22" height="40" rx="9" fill="{TEAL}" stroke="#000" stroke-opacity=".5" stroke-width="2"/>
      <rect x="28" y="0" width="22" height="40" rx="9" fill="{TEAL}" stroke="#000" stroke-opacity=".5" stroke-width="2"/>
      <g fill="none" stroke="{ORANGE}" stroke-width="3" stroke-linecap="round">
        <path class="sound s1" d="M58,8 Q66,20 58,32"/><path class="sound s2" d="M66,2 Q78,20 66,38"/>
        <path class="sound s1" d="M-58,8 Q-66,20 -58,32"/><path class="sound s2" d="M-66,2 Q-78,20 -66,38"/>
      </g>
    </g>'''

    cables = []
    accents = [TEAL, TEAL, TEAL, ORANGE]
    for i in range(4):
        x0, x1 = xs[i] + card / 2, xs[i + 1] - card / 2
        cables.append(f'<line x1="{x0}" y1="{cy}" x2="{x1}" y2="{cy}" stroke="#34343a" stroke-width="8" stroke-linecap="round"/>')
        cables.append(f'<line class="flow" x1="{x0}" y1="{cy}" x2="{x1}" y2="{cy}" stroke="{accents[i]}" stroke-width="4" stroke-linecap="round" stroke-dasharray="4 14"/>')

    labels = [("1", "Plug in", "your guitar"), ("2", "Audio", "interface"), ("3", "Your DAW", "audio track"),
              ("4", "OAP plugins", "amp, pedals, board"), ("5", "Rock out!", "headphones / monitors")]
    lab = []
    for (n, a, b), x in zip(labels, xs):
        acc = ORANGE if n in ("4", "5") else TEAL
        lab.append(f'<circle cx="{x-card/2+6}" cy="{cy-card/2+6}" r="17" fill="{acc}"/>'
                   f'<text x="{x-card/2+6}" y="{cy-card/2+12}" text-anchor="middle" font-family="{FONT}" font-weight="800" font-size="17" fill="#111">{n}</text>')
        lab.append(f'<text x="{x}" y="{cy+card/2+34}" text-anchor="middle" font-family="{FONT}" font-weight="700" font-size="19" fill="#f0f0f0">{a}</text>')
        lab.append(f'<text x="{x}" y="{cy+card/2+56}" text-anchor="middle" font-family="{FONT}" font-size="15" fill="#9a9ca3">{b}</text>')

    s = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" aria-label="Guitar into audio interface into a DAW audio track, through the OAP plugins, out to headphones">
<title>How the signal flows</title>
<style>
  .flow {{ animation: flow .9s linear infinite; }}
  @keyframes flow {{ to {{ stroke-dashoffset: -18; }} }}
  .knob {{ transform-box: fill-box; transform-origin: center; animation: wiggle 2.4s ease-in-out infinite alternate; }}
  @keyframes wiggle {{ 0% {{ transform: rotate(-110deg); }} 100% {{ transform: rotate(110deg); }} }}
  .glow {{ transform-box: fill-box; transform-origin: center; animation: pulse 1.4s ease-in-out infinite; }}
  @keyframes pulse {{ 0%,100% {{ opacity: .15; transform: scale(.9); }} 50% {{ opacity: .7; transform: scale(1.6); }} }}
  .strum {{ animation: strum .16s linear infinite alternate; }}
  @keyframes strum {{ from {{ transform: translateX(-1px); }} to {{ transform: translateX(1px); }} }}
  .blink {{ animation: blink 1s steps(2) infinite; }}
  @keyframes blink {{ 50% {{ opacity: .25; }} }}
  .bar {{ transform-box: fill-box; transform-origin: bottom; animation: eq .7s ease-in-out infinite alternate; }}
  .b2 {{ animation-delay: -.2s; }} .b3 {{ animation-delay: -.45s; }} .b4 {{ animation-delay: -.1s; }} .b5 {{ animation-delay: -.6s; }}
  @keyframes eq {{ 0% {{ transform: scaleY(.2); }} 100% {{ transform: scaleY(1); }} }}
  .sound {{ animation: sound 1.2s ease-out infinite; }}
  .s2 {{ animation-delay: .3s; }}
  @keyframes sound {{ 0% {{ opacity: 0; }} 30% {{ opacity: 1; }} 100% {{ opacity: 0; }} }}
  {REDUCED}
</style>
<rect width="{W}" height="{H}" rx="26" fill="#121215"/>
<rect x=".5" y=".5" width="{W-1}" height="{H-1}" rx="26" fill="none" stroke="#fff" stroke-opacity=".07"/>
{"".join(cables)}
{card_rect(xs[0], TEAL)}{card_rect(xs[1], TEAL)}{card_rect(xs[2], TEAL)}{card_rect(xs[3], ORANGE)}{card_rect(xs[4], ORANGE)}
{guitar}
{interface}
{laptop}
{amp}
{phones}
{"".join(lab)}
</svg>
'''
    return s


# ---------------------------------------------------------------------------
def terminal():
    W = 1000
    lines = [
        ("cmd", "brew install cmake ninja"),
        ("out", "✔ cmake and ninja installed", "#4cd964"),
        ("cmd", "git clone https://github.com/mitradeniz/FreeAudioFX.git"),
        ("cmd", "cd FreeAudioFX"),
        ("cmd", "cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"),
        ("out", "-- Downloading JUCE 9.0.5 ... done", "#9a9ca3"),
        ("cmd", "cmake --build build"),
        ("bar",),
        ("out", "✔ 12 amps (VST3 + AU) → ~/Library/Audio/Plug-Ins", "#ff7a45"),
        ("out", "✔ 20 pedals + Pedalboard → ~/Library/Audio/Plug-Ins", "#2fd1bf"),
        ("out", "Done! Open your DAW and rescan your plug-ins.", "#f5f5f5"),
    ]
    CW, LH, X0, Y0 = 9.6, 30, 36, 86
    TOTAL = 18.0
    t = 0.6
    css, body = [], []
    H = Y0 + LH * len(lines) + 20
    for i, ln in enumerate(lines):
        y = Y0 + i * LH
        kind = ln[0]
        if kind == "cmd":
            text = ln[1]
            n = len(text)
            dur = max(0.5, n * 0.035)
            start, end = t, t + dur
            # line visible from start; cover slides right revealing characters
            a0, a1 = start / TOTAL * 100, end / TOTAL * 100
            css.append(f".l{i} {{ animation: show{i} {TOTAL}s linear infinite; }}"
                       f"@keyframes show{i} {{ 0%, {a0:.2f}% {{ opacity: 0; }} {a0+0.01:.2f}%, 97% {{ opacity: 1; }} 100% {{ opacity: 0; }} }}")
            css.append(f".c{i} {{ animation: type{i} {TOTAL}s steps(1, end) infinite; }}")
            # build stepped keyframes so characters appear one by one
            steps = []
            for k in range(n + 1):
                pct = (start + dur * k / n) / TOTAL * 100
                steps.append(f"{pct:.3f}% {{ transform: translateX({k*CW:.1f}px); }}")
            css.append(f"@keyframes type{i} {{ 0% {{ transform: translateX(0); }} {' '.join(steps)} 100% {{ transform: translateX({n*CW:.1f}px); }} }}")
            body.append(f'<g class="l{i}"><text x="{X0}" y="{y}" font-family="{MONO}" font-size="16" fill="#ff7a45" font-weight="700">$</text>'
                        f'<text x="{X0+22}" y="{y}" font-family="{MONO}" font-size="16" fill="#f0f0f0" textLength="{n*CW:.1f}" lengthAdjust="spacingAndGlyphs">{text.replace("&","&amp;")}</text>'
                        f'<rect class="c{i}" x="{X0+22}" y="{y-18}" width="{n*CW+14:.1f}" height="26" fill="#18181c"/>'
                        f'<rect class="c{i} k{i}" x="{X0+22}" y="{y-15}" width="9" height="19" fill="#f0f0f0" opacity=".8"/></g>')
            c0, c1 = start / TOTAL * 100, (end + 0.4) / TOTAL * 100
            css.append(f".k{i} {{ animation: type{i} {TOTAL}s steps(1, end) infinite, cur{i} {TOTAL}s steps(1, end) infinite; }}"
                       f"@keyframes cur{i} {{ 0% {{ opacity: 0; }} {c0:.2f}% {{ opacity: .85; }} {c1:.2f}% {{ opacity: 0; }} 100% {{ opacity: 0; }} }}")
            t = end + 0.45
        elif kind == "out":
            text, col = ln[1], ln[2]
            a0 = t / TOTAL * 100
            css.append(f".l{i} {{ animation: show{i} {TOTAL}s linear infinite; }}"
                       f"@keyframes show{i} {{ 0%, {a0:.2f}% {{ opacity: 0; }} {a0+1.2:.2f}%, 97% {{ opacity: 1; }} 100% {{ opacity: 0; }} }}")
            body.append(f'<text class="l{i}" x="{X0+22}" y="{y}" font-family="{MONO}" font-size="16" fill="{col}">{text}</text>')
            t += 0.55
        elif kind == "bar":
            dur = 3.0
            a0, a1 = t / TOTAL * 100, (t + dur) / TOTAL * 100
            css.append(f".l{i} {{ animation: show{i} {TOTAL}s linear infinite; }}"
                       f"@keyframes show{i} {{ 0%, {a0:.2f}% {{ opacity: 0; }} {a0+0.5:.2f}%, 97% {{ opacity: 1; }} 100% {{ opacity: 0; }} }}")
            css.append(f".fill {{ transform-box: fill-box; transform-origin: left; animation: fill {TOTAL}s ease-in-out infinite; }}"
                       f"@keyframes fill {{ 0%, {a0:.2f}% {{ transform: scaleX(0); }} {a1:.2f}%, 100% {{ transform: scaleX(1); }} }}")
            body.append(f'<g class="l{i}"><rect x="{X0+22}" y="{y-14}" width="560" height="16" rx="8" fill="#2a2a30"/>'
                        f'<rect class="fill" x="{X0+22}" y="{y-14}" width="560" height="16" rx="8" fill="url(#prog)"/>'
                        f'<text x="{X0+600}" y="{y}" font-family="{MONO}" font-size="15" fill="#9a9ca3">building plugins...</text></g>')
            t += dur + 0.3
    assert t < TOTAL - 1.5, t

    s = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" aria-label="Terminal: install CMake and Ninja, clone the repository, build the plugins">
<title>Building in four commands</title>
<style>
{chr(10).join(css)}
{REDUCED}
@media (prefers-reduced-motion: reduce) {{ [class^="l"] {{ opacity: 1 !important; }} [class^="c"] {{ display: none; }} .fill {{ transform: none !important; }} }}
</style>
<defs><linearGradient id="prog" x1="0" x2="1"><stop offset="0" stop-color="{TEAL}"/><stop offset="1" stop-color="{ORANGE}"/></linearGradient></defs>
<rect width="{W}" height="{H}" rx="16" fill="#18181c"/>
<rect x=".5" y=".5" width="{W-1}" height="{H-1}" rx="16" fill="none" stroke="#fff" stroke-opacity=".09"/>
<path d="M0,16 Q0,0 16,0 L{W-16},0 Q{W},0 {W},16 L{W},44 L0,44 Z" fill="#26262b"/>
<circle cx="26" cy="22" r="7" fill="#ff5f57"/><circle cx="48" cy="22" r="7" fill="#febc2e"/><circle cx="70" cy="22" r="7" fill="#28c840"/>
<text x="{W/2}" y="27" text-anchor="middle" font-family="{FONT}" font-size="14" fill="#9a9ca3">Terminal — FreeAudioFX</text>
{chr(10).join(body)}
</svg>
'''
    return s


os.makedirs(OUT, exist_ok=True)
for name, fn in (("hero.svg", hero), ("signal-flow.svg", signal_flow), ("terminal.svg", terminal)):
    with open(os.path.join(OUT, name), "w") as f:
        f.write(fn())
    print(name, os.path.getsize(os.path.join(OUT, name)))
