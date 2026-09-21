# Corrupt World

A small walkable 3D "corrupted" terrain demo written in **C** with **GLFW**
and legacy (fixed-function) **OpenGL**. The heightfield terrain is driven by
a cheap pseudo-noise function that occasionally spikes into glitchy
magenta/red shards, and the corruption pattern slowly drifts as you walk.

## Controls

| Key            | Action              |
|----------------|---------------------|
| `W` `A` `S` `D`| Move                |
| Mouse          | Look around         |
| `Space`        | Move up             |
| `Left Shift`   | Move down           |
| `R`            | Re-roll the corruption seed |
| `Tab`          | Toggle mouse capture |
| `Esc`          | Quit                |

## Building

### Linux / macOS

Install dependencies:

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev

# macOS (Homebrew)
brew install cmake glfw
```

Build:

```bash
cmake -B build -S .
cmake --build build
./build/corrupt_world
```

### Windows

Install [CMake](https://cmake.org/) and [GLFW](https://www.glfw.org/), then:

```powershell
cmake -B build -S .
cmake --build build --config Release
```

## How it works

- The terrain is a `GRID_SIZE x GRID_SIZE` grid of vertices rendered as
  triangle strips.
- Each vertex's height comes from a hashed/interpolated noise function
  (`corrupt_noise`) that drifts over time.
- A small fraction of grid cells roll a "glitch spike," jumping the height
  and flashing the color to hot magenta — this is the "corruption."
- Floating glitch shards are scattered above the terrain for extra flavor.
- The camera walks along the terrain surface (with a little headroom) and
  can also fly up/down freely.

## License

MIT — do whatever you like with it.
