import zlib, struct

S = 512

def in_rrect(px, py, x0, y0, x1, y1, r):
    if px < x0 or px > x1 or py < y0 or py > y1:
        return False
    cx = min(max(px, x0 + r), x1 - r)
    cy = min(max(py, y0 + r), y1 - r)
    return (px - cx) ** 2 + (py - cy) ** 2 <= r * r

def incirc(px, py, cx, cy, r):
    return (px - cx) ** 2 + (py - cy) ** 2 <= r * r

# palette
BG    = (32, 41, 30)     # dark olive (distinct from the Unity icon's slate)
TAN   = (203, 173, 122)  # cardboard
EDGE  = (150, 124, 82)   # darker cardboard edge/outline
AZONE = (74, 60, 42)     # perforated A-zone
HOLE  = (22, 20, 17)     # bullet holes

holes = [(238, 300, 10), (274, 286, 10), (256, 332, 9), (262, 250, 9)]

def px_color(x, y):
    # USPSA "metric" target: a wide rounded body + a smaller head bump.
    body  = in_rrect(x, y, 120, 200, 392, 436, 48)
    head  = in_rrect(x, y, 198, 112, 314, 214, 40)
    body_e = in_rrect(x, y, 112, 192, 400, 444, 52)
    head_e = in_rrect(x, y, 190, 104, 322, 222, 44)

    c = BG
    if body_e or head_e: c = EDGE          # thin outline
    if body or head:     c = TAN           # cardboard
    # A-zones (scoring center)
    if in_rrect(x, y, 214, 232, 298, 356, 14): c = AZONE   # body A
    if in_rrect(x, y, 230, 144, 282, 198, 10): c = AZONE   # head A
    # bullet holes
    for hx, hy, hr in holes:
        if incirc(x, y, hx, hy, hr): c = HOLE
    return c

raw = bytearray()
for y in range(S):
    raw.append(0)
    for x in range(S):
        raw += bytes(px_color(x, y))

def chunk(typ, data):
    return (struct.pack(">I", len(data)) + typ + data +
            struct.pack(">I", zlib.crc32(typ + data) & 0xffffffff))

png = (b'\x89PNG\r\n\x1a\n'
       + chunk(b'IHDR', struct.pack(">IIBBBBB", S, S, 8, 2, 0, 0, 0))
       + chunk(b'IDAT', zlib.compress(bytes(raw), 9))
       + chunk(b'IEND', b''))
open('/tmp/uspsa_target_icon.png', 'wb').write(png)
print("wrote /tmp/uspsa_target_icon.png", S, "x", S)
