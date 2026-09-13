# Igor raster assets

These are deterministic scaled variants of the canonical mascot at
[`/igor.png`](../../igor.png). The original is never overwritten.

Source:

- file: `igor.png`
- dimensions: 1254×1254 RGBA
- SHA-256: `a9dae7b7d9b0c19bcdd27cd43c17f4b88bdd3e798077bbf645522416fcd39514`

Generated with ImageMagick using Lanczos resampling and PNG compression:

```text
convert igor.png -filter Lanczos -resize NxN -define png:compression-level=9 igor-N.png
```

Available sizes:

```text
16  24  32  48  64  96  128  180  192  256  384  512  768  1024
```

Use 16–128 for interface and launcher icons, 180/192/256 for application and
platform icon families, and 384–1024 for documents, README material, and
larger rendered layouts. All variants retain a transparent background.
