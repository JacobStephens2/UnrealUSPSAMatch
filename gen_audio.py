#!/usr/bin/env python3
"""Synthesize the game's WAV assets with the standard library only."""
import math, os, struct, wave, random

SR = 44100
OUT = os.path.expanduser("~/UnrealProjects/Shooter3D/AudioSrc")
os.makedirs(OUT, exist_ok=True)
random.seed(7)


def write_wav(name, samples):
    path = os.path.join(OUT, name + ".wav")
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        frames = bytearray()
        for s in samples:
            v = int(max(-1.0, min(1.0, s)) * 32767)
            frames += struct.pack("<h", v)
        w.writeframes(frames)
    print("wrote", path, len(samples), "samples")


def env(i, n, attack=0.01, release=0.2):
    a = int(attack * SR)
    r = int(release * SR)
    if i < a:
        return i / max(a, 1)
    if i > n - r:
        return max(0.0, (n - i) / max(r, 1))
    return 1.0


# --- buzzer: harsh ~2300 Hz square tone (the range timer "beep") ---
def buzzer():
    n = int(0.45 * SR)
    out = []
    for i in range(n):
        t = i / SR
        sq = 1.0 if math.sin(2 * math.pi * 2300 * t) > 0 else -1.0
        sq += 0.4 * (1.0 if math.sin(2 * math.pi * 3450 * t) > 0 else -1.0)
        out.append(0.5 * sq * env(i, n, 0.004, 0.05))
    return out


# --- shot: noise burst + low thump, fast decay ---
def shot():
    n = int(0.16 * SR)
    out = []
    for i in range(n):
        t = i / SR
        decay = math.exp(-t * 38)
        noise = (random.random() * 2 - 1) * decay
        thump = math.sin(2 * math.pi * 75 * t) * math.exp(-t * 28) * 0.8
        out.append(max(-1.0, min(1.0, 0.9 * noise + thump)))
    return out


# --- steel: metallic ring (two partials) ---
def steel():
    n = int(0.5 * SR)
    out = []
    for i in range(n):
        t = i / SR
        d = math.exp(-t * 9)
        s = math.sin(2 * math.pi * 1500 * t) + 0.6 * math.sin(2 * math.pi * 2240 * t)
        out.append(0.45 * s * d)
    return out


# --- paper: short dull thwack ---
def paper():
    n = int(0.10 * SR)
    out = []
    prev = 0.0
    for i in range(n):
        t = i / SR
        raw = (random.random() * 2 - 1)
        prev = prev * 0.6 + raw * 0.4  # cheap low-pass
        out.append(0.5 * prev * math.exp(-t * 45))
    return out


# --- music: looping minor-key driving bed (bass + arpeggio) ---
def music():
    bpm = 132
    beat = 60.0 / bpm
    step = beat / 2          # eighth notes
    steps = 32               # 4 bars
    # A natural minor-ish pattern.
    def hz(semi):  # semitones above A2 (110 Hz)
        return 110.0 * (2 ** (semi / 12.0))
    bass_pat = [0, 0, 7, 0, 5, 5, 3, 0, 0, 0, 7, 0, 8, 7, 5, 3,
                0, 0, 7, 0, 5, 5, 3, 0, -2, -2, 3, -2, 0, 3, 5, 7]
    arp_pat = [12, 15, 19, 24, 19, 15, 12, 15, 17, 12, 15, 19, 15, 12, 10, 12,
               12, 15, 19, 24, 19, 15, 12, 15, 10, 13, 17, 13, 12, 15, 19, 22]
    n = int(steps * step * SR)
    out = [0.0] * n
    for s in range(steps):
        start = int(s * step * SR)
        ln = int(step * SR)
        bf = hz(bass_pat[s % len(bass_pat)] - 12)
        af = hz(arp_pat[s % len(arp_pat)])
        for i in range(ln):
            idx = start + i
            if idx >= n:
                break
            t = i / SR
            e = env(i, ln, 0.005, step * 0.5)
            bass = (1.0 if math.sin(2 * math.pi * bf * t) > 0 else -1.0) * 0.22
            arp = math.sin(2 * math.pi * af * t) * 0.16 * e
            out[idx] += bass * e + arp
    # gentle clip
    return [max(-1.0, min(1.0, v)) for v in out]


write_wav("buzzer", buzzer())
write_wav("shot", shot())
write_wav("steel", steel())
write_wav("paper", paper())
write_wav("music", music())
print("done")
