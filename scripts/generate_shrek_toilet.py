from pathlib import Path

w, h = 512, 512
pixels = []

for y in range(h):
    row = []
    for x in range(w):
        nx = (x / w) - 0.5
        ny = (y / h) - 0.5

        r = int(25 + 20 * (1 + nx * 0.5))
        g = int(80 + 60 * (0.5 + ny * 0.2))
        b = int(35 + 18 * (1 - abs(nx) * 0.8))

        stripe = (x + y * 7) % 53
        if stripe < 8:
            r = int(r * 0.7)
            g = int(g * 0.8)
            b = int(b * 0.7)

        dist = ((x - w / 2) / (w * 0.38)) ** 2 + ((y - h / 2) / (h * 0.32)) ** 2
        if dist < 1.0:
            r, g, b = 228, 220, 185
        if dist < 0.72 and (x < w * 0.52 or x > w * 0.48):
            r, g, b = 200, 190, 155

        dist2 = ((x - w / 2) / (w * 0.46)) ** 2 + ((y - h * 0.68) / (h * 0.21)) ** 2
        if dist2 < 1.0:
            r = max(0, r - 60)
            g = max(0, g - 50)
            b = max(0, b - 55)

        for cx, cy, rad in [(w * 0.23, h * 0.3, 65), (w * 0.74, h * 0.6, 75), (w * 0.6, h * 0.18, 52)]:
            dx = (x - cx) / rad
            dy = (y - cy) / rad
            if dx * dx + dy * dy < 1.0:
                r = min(255, r + 45)
                g = min(255, g + 90)
                b = min(255, b + 35)

        if (x ^ y) % 29 == 0:
            r = max(0, r - 18)
            g = max(0, g - 25)
            b = max(0, b - 20)

        row.append((max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b))))
    pixels.append(row)

path = Path(__file__).resolve().parent.parent / "textures" / "shrek_toilet.ppm"
path.parent.mkdir(parents=True, exist_ok=True)
with path.open("wb") as f:
    f.write(f"P6\n{w} {h}\n255\n".encode("ascii"))
    for row in pixels:
        for pixel in row:
            f.write(bytes(pixel))

print(f"Generated: {path}")
print(f"Size: {path.stat().st_size} bytes")
