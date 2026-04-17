#!/usr/bin/env python3
"""Optimal teletype overstrike art for the Model 35."""

import argparse
import os
import time
import numpy as np
from PIL import Image, ImageFont, ImageDraw

COLS = 72
FONT_SIZE = 20
EDGE_BOOST = 2.0
COL_INCHES = 0.10  # Model 35 horizontal pitch
ROW_INCHES = 0.16  # Model 35 vertical pitch


def linear_to_Lstar(Y):
    """CIE L* from linear luminance Y in [0,1]."""
    Yn = np.clip(Y, 0, 1)
    f = np.where(Yn > 0.008856, Yn ** (1 / 3), 7.787 * Yn + 16 / 116)
    return 116 * f - 16


PAPER_L = linear_to_Lstar(1.0)


def extract_glyphs(font_path):
    """Render printable ASCII glyphs into fixed-size bitmaps.
    Returns (masks[n,h,w] in [0,1], chars, cell_h, cell_w)."""
    font = ImageFont.truetype(font_path, FONT_SIZE)
    # Model 35 TTY: 63 printable chars (codes 33-95).
    # 33-93 are standard ASCII, 94=up-arrow, 95=left-arrow. No lowercase.
    chars = [chr(c) for c in range(0x21, 0x60)]

    bboxes = [font.getbbox(ch) for ch in chars]
    cell_w = max(b[2] - b[0] for b in bboxes)
    cell_h = max(b[3] - b[1] for b in bboxes)

    masks = []
    for ch, bbox in zip(chars, bboxes):
        img = Image.new("L", (cell_w, cell_h), 0)
        ImageDraw.Draw(img).text((-bbox[0], -bbox[1]), ch, fill=255, font=font)
        masks.append(np.array(img, dtype=np.float64) / 255.0)

    return np.array(masks), chars, cell_h, cell_w


def prepare_target(
    image_path,
    rows,
    row_pitch,
    col_pitch,
    gamma=1.0,
    levels=False,
    highlight_compress=0.0,
):
    """Load image, convert to grayscale, resize to page grid.
    Returns float64 array in [0,1] (0=black, 1=white)."""
    img = Image.open(image_path).convert("L")
    img = img.resize((COLS * col_pitch, rows * row_pitch), Image.Resampling.LANCZOS)
    arr = np.array(img, dtype=np.float64) / 255.0

    if levels:
        lo, hi = np.percentile(arr[arr < 0.99], [1, 99])
        if hi > lo:
            arr = np.clip((arr - lo) / (hi - lo), 0, 1)

    if highlight_compress > 0:
        face = arr[arr < 0.95]
        knee = np.median(face) if len(face) > 0 else 0.5
        mask = arr > knee
        above = (arr[mask] - knee) / (1.0 - knee)
        arr[mask] = knee + (1.0 - knee) * np.power(above, highlight_compress)

    if gamma != 1.0:
        arr = np.power(arr, gamma)

    return arr


def compute_edge_weights(target_dark):
    """Per-pixel importance from Sobel gradient magnitude.
    Returns weights in [1, 1+EDGE_BOOST]."""
    dy = np.zeros_like(target_dark)
    dx = np.zeros_like(target_dark)
    dy[1:-1] = (target_dark[2:] - target_dark[:-2]) / 2
    dx[:, 1:-1] = (target_dark[:, 2:] - target_dark[:, :-2]) / 2
    dy[0] = target_dark[1] - target_dark[0]
    dy[-1] = target_dark[-1] - target_dark[-2]
    dx[:, 0] = target_dark[:, 1] - target_dark[:, 0]
    dx[:, -1] = target_dark[:, -1] - target_dark[:, -2]

    grad = np.sqrt(dx**2 + dy**2)
    gmax = grad.max()
    return 1.0 + EDGE_BOOST * (grad / gmax if gmax > 0 else grad)


def greedy_solve_cell(target_dark, glyph_masks, alpha, budget, weights):
    """Greedy solver: each strike picks the glyph that most reduces weighted L2 error.
    Returns (counts[n_glyphs], achieved_dark[h,w])."""
    n_glyphs = len(glyph_masks)
    masks_flat = glyph_masks.reshape(n_glyphs, -1)
    target_flat = target_dark.ravel()
    w = weights.ravel()

    counts = np.zeros(n_glyphs, dtype=int)
    exposure = np.zeros_like(target_dark)

    for _ in range(budget):
        cur_dark = (PAPER_L - linear_to_Lstar(np.exp(-alpha * exposure))).ravel()
        cur_err = np.sum(w * (target_flat - cur_dark) ** 2)

        cand_exp = exposure.ravel()[None, :] + masks_flat
        cand_dark = PAPER_L - linear_to_Lstar(np.exp(-alpha * cand_exp))
        cand_err = np.sum(w[None, :] * (target_flat[None, :] - cand_dark) ** 2, axis=1)

        best = np.argmin(cand_err)
        if cand_err[best] >= cur_err:
            break
        counts[best] += 1
        exposure += glyph_masks[best]

    achieved = PAPER_L - linear_to_Lstar(np.exp(-alpha * exposure))
    return counts, achieved


def build_fs_kernels(cell_h, cell_w, col_pitch, row_pitch):
    """Spatial diffusion kernels, distance-weighted for physical cell spacing."""
    col_ramp = np.broadcast_to(np.linspace(0, 1, cell_w)[None, :], (cell_h, cell_w))
    row_ramp = np.broadcast_to(np.linspace(0, 1, cell_h)[:, None], (cell_h, cell_w))
    d_d = np.sqrt(col_pitch**2 + row_pitch**2)

    k = [
        col_ramp / col_pitch,
        row_ramp * (1 - col_ramp) / d_d,
        row_ramp / row_pitch,
        row_ramp * col_ramp / d_d,
    ]

    total = sum(ki.sum() for ki in k)
    return tuple(ki / total for ki in k)


def solve(glyph_masks, rows, alpha, budget, col_pitch, row_pitch, target):
    """Greedy solver with spatial Floyd-Steinberg error diffusion."""
    n_glyphs, cell_h, cell_w = glyph_masks.shape
    k_r, k_bl, k_b, k_br = build_fs_kernels(cell_h, cell_w, col_pitch, row_pitch)

    # Target darkness per cell
    cell_targets = np.zeros((rows, COLS, cell_h, cell_w))
    cell_weights = np.zeros((rows, COLS, cell_h, cell_w))
    for r in range(rows):
        for c in range(COLS):
            y0, x0 = r * row_pitch, c * col_pitch
            cell_targets[r, c] = PAPER_L - linear_to_Lstar(
                target[y0 : y0 + cell_h, x0 : x0 + cell_w]
            )
            cell_weights[r, c] = compute_edge_weights(cell_targets[r, c])

    error_acc = np.zeros((rows, COLS, cell_h, cell_w))
    n_int = np.zeros(rows * COLS * n_glyphs, dtype=int)

    t0 = time.time()
    for r in range(rows):
        for c in range(COLS):
            ci = r * COLS + c
            effective = np.clip(cell_targets[r, c] + error_acc[r, c], 0, None)

            cell_int, achieved = greedy_solve_cell(
                effective, glyph_masks, alpha, budget, cell_weights[r, c]
            )
            n_int[ci * n_glyphs : (ci + 1) * n_glyphs] = cell_int

            residual = effective - achieved
            if c + 1 < COLS:
                error_acc[r, c + 1] += residual * k_r
            if r + 1 < rows:
                if c - 1 >= 0:
                    error_acc[r + 1, c - 1] += residual * k_bl
                error_acc[r + 1, c] += residual * k_b
                if c + 1 < COLS:
                    error_acc[r + 1, c + 1] += residual * k_br

        print(f"  row {r+1}/{rows}  ({time.time()-t0:.1f}s)")

    return n_int


def generate_output(n_int, glyph_chars, n_glyphs, rows):
    """Convert glyph counts to overstrike byte stream (\\r within row, \\r\\n between)."""
    lines = []
    for row in range(rows):
        remaining = {}
        for col in range(COLS):
            ci = row * COLS + col
            cell = {}
            for gi in range(n_glyphs):
                count = n_int[ci * n_glyphs + gi]
                if count > 0:
                    cell[glyph_chars[gi]] = count
            remaining[col] = cell

        max_passes = max(
            (sum(remaining[col].values()) for col in range(COLS)), default=0
        )

        passes = []
        for _ in range(max_passes):
            chars = []
            any_ink = False
            for col in range(COLS):
                cell = remaining[col]
                if cell:
                    best = max(cell, key=cell.get)
                    chars.append(best)
                    cell[best] -= 1
                    if cell[best] == 0:
                        del cell[best]
                    any_ink = True
                else:
                    chars.append(" ")
            if any_ink:
                passes.append("".join(chars).rstrip())
        if passes:
            lines.append("\r".join(passes))
    return "\r\n".join(lines)


def render_preview(
    n_int, glyph_masks, n_glyphs, rows, cell_h, cell_w, row_pitch, col_pitch, alpha
):
    """Render Beer-Lambert density preview image."""
    exposure = np.zeros((rows * row_pitch, COLS * col_pitch))
    n_mat = n_int.reshape(rows * COLS, n_glyphs)

    for ci in range(rows * COLS):
        r, c = divmod(ci, COLS)
        y0, x0 = r * row_pitch, c * col_pitch
        for gi in range(n_glyphs):
            if n_mat[ci, gi] > 0:
                exposure[y0 : y0 + cell_h, x0 : x0 + cell_w] += (
                    n_mat[ci, gi] * glyph_masks[gi]
                )

    brightness = ((np.exp(-alpha * exposure)) * 255).clip(0, 255).astype(np.uint8)
    return Image.fromarray(brightness, "L")


def main():
    p = argparse.ArgumentParser(description="Optimal Teletype Overstrike Art")
    p.add_argument("image")
    p.add_argument("--alpha", type=float, default=1.0)
    p.add_argument("--budget", type=int, default=16)
    p.add_argument("--font", default=None)
    p.add_argument("--output", default="output.txt")
    p.add_argument("--preview", default="preview.png")
    p.add_argument("--gamma", type=float, default=1.0)
    p.add_argument("--levels", action="store_true")
    p.add_argument("--highlight-compress", type=float, default=0.0)
    args = p.parse_args()

    if args.font is None:
        args.font = os.path.join(os.path.dirname(__file__), "TTY35TD-Book.ttf")

    # Extract glyphs and compute layout
    glyph_masks, glyph_chars, cell_h, cell_w = extract_glyphs(args.font)
    n_glyphs = len(glyph_masks)
    dpi = round(cell_w / COL_INCHES)
    col_pitch = round(COL_INCHES * dpi)
    row_pitch = round(ROW_INCHES * dpi)

    # Auto rows from image aspect ratio
    w, h = Image.open(args.image).size
    rows = round(h / w * COLS * col_pitch / row_pitch)

    print(
        f"  {n_glyphs} glyphs, cell {cell_w}x{cell_h}, "
        f"pitch {col_pitch}x{row_pitch}, {COLS}x{rows} cells"
    )

    # Prepare target
    target = prepare_target(
        args.image,
        rows,
        row_pitch,
        col_pitch,
        args.gamma,
        args.levels,
        args.highlight_compress,
    )

    # Solve
    print(f"Solving (budget={args.budget})...")
    n_int = solve(
        glyph_masks, rows, args.alpha, args.budget, col_pitch, row_pitch, target
    )

    # Output
    output = generate_output(n_int, glyph_chars, n_glyphs, rows)
    with open(args.output, "w") as f:
        f.write(output)

    preview = render_preview(
        n_int,
        glyph_masks,
        n_glyphs,
        rows,
        cell_h,
        cell_w,
        row_pitch,
        col_pitch,
        args.alpha,
    )
    preview.save(args.preview)

    total = n_int.sum()
    active = np.sum(n_int.reshape(-1, n_glyphs).sum(axis=1) > 0)
    print(f"  {total} strikes, {active}/{rows*COLS} active cells")
    print(f"  -> {args.output}, {args.preview}")


if __name__ == "__main__":
    main()
