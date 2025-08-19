# Lunaforge
Lunaforge is a game development toolchain specifically designed for certain embedded device platforms.
It is currently still under development, and documentation is incomplete.

The goal is to write core components in standard C89, with some components implemented in TypeScript.

## Components
| Component | Description |
|---|---|
| Runeshard (碎灵符) | Sprite sheet slicing tool that converts assets into 8x8 tile collections |
| Bonegaze (照骨镜) | Project build tool, similar to `make` |
| Khronicler (董狐笔) | K-Basic scripting language interpreter |
| Gopuzzle (珍珑局) | Map editor supporting GDI or SDL as graphics backends |

## Artifacts
| Artifact | Description |
|---|---|
| jasmine89 | JSON parsing tool used for parsing configuration and project files |
| salvia89 | Dependency-free sprintf replacement |
| raster-fonts | Rasterized pixel fonts |
| key-code-tester | Key code testing utility for WinCE |
| color-palette | VGA palette definitions |
| color-wheel-maker | Color wheel SVG generation tool |
