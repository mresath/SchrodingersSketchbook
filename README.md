# Schrödinger's Sketchbook

A physics simulator/playground with the following features:

- Simple 2D graphics
- Easy to use UI
- Interactive quantum concepts:
  - Wave packet initialization
  - User-drawn potential barriers
  - Center wall and double-slit presets
  - Probability-density visualization
- Adjustable simulation settings:
  - Time step
  - Substeps
  - Solver iterations
  - Coordinate scale
  - Renormalization toggle

This simulation was initially created as a supplemental for my AP Physics C class end of the year presentation.

Check [SCIENCE.md](SCIENCE.md) on how the simulation works.

## Installation

Download the latest release for your operating system and launch.

## Controls

- `LMB`: paint potential
- `Shift + LMB`: erase potential
- `RMB + drag`: pan view
- `Mouse wheel`: zoom
- `MMB`: reset view
- `P`: pause/resume simulation
- `R`: reset packet from current packet settings
- `C`: clear potential field

UI panel (`Sandbox`):

- Pause/Resume and single-step
- Reset packet and clear potential
- Presets: center wall, double slit
- Simulation controls: `Sim dt`, `Substeps`, `Solver iters`, `Coord scale`, `Hbar`, renormalization toggle
- Interaction controls: barrier strength, brush radius, overlay toggle
- Packet controls: center, momentum, sigma, mass

## Notes

- The latest version of imgui-sfml requires imgui 1.91.x, so to compile the project you need to checkout v1.91.9b for that submodule.

## Copyright

Distributed under the GNU GPLv3 License. See `LICENSE` for more information.
