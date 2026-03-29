# Evolution Simulator

A real-time natural selection simulation built with C++ and SFML. Creatures roam a 2D world, eat food, reproduce with mutation, and die — letting evolution play out live on screen.

![C++](https://img.shields.io/badge/C++-17-blue?logo=c%2B%2B) ![SFML](https://img.shields.io/badge/SFML-3.x-green) ![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

---

## How It Works

Every creature has three heritable traits:

| Trait | Effect |
|-------|--------|
| **Size** | Bigger creatures eat food more easily but burn energy faster |
| **Speed** | Faster creatures cover more ground but also cost more energy |
| **Color** | Drifts each generation — a visual trace of lineage |

Each frame, creatures move around bouncing off walls, burning energy just by existing. When a creature eats enough food to reach **200 energy**, it reproduces — cloning itself with small random mutations applied to all three traits. If energy hits **zero**, the creature dies.

Food slowly regrows each frame (10% chance per frame, capped at 200 pellets), creating natural boom-and-bust population cycles.

---

## Features

- Real-time evolution with visible mutation across generations
- Energy system where size and speed have genuine trade-offs
- Food regrowth creating dynamic population pressure
- Live stats sidebar showing population, peak pop, food count, total births, average size and speed

---

## Project Structure

```
Evolution/
├── main.cpp          # Window setup and main loop
├── Simulation.h/cpp  # Core update logic — spawning, eating, death
├── Creature.h/cpp    # Movement, energy burn, reproduction & mutation
├── Food.h/cpp        # Food spawning
├── UI.h/cpp          # Sidebar stats panel
└── .vscode/
    ├── tasks.json            # Build task
    └── c_cpp_properties.json # IntelliSense config
```

---

## Requirements

- [MSYS2](https://www.msys2.org/) with the UCRT64 toolchain
- SFML 3.x installed via MSYS2

Install SFML through the MSYS2 terminal:

```bash
pacman -S mingw-w64-ucrt-x86_64-sfml
```

---

## Building

Open the project folder in VS Code and press `Ctrl+Shift+B` to run the default build task. This compiles all source files and outputs `Evolution.exe`.

Or build manually from the terminal:

```bash
g++ -g main.cpp Simulation.cpp Creature.cpp Food.cpp UI.cpp \
    -o Evolution.exe \
    -I C:/msys64/ucrt64/include \
    -L C:/msys64/ucrt64/lib \
    -lsfml-graphics -lsfml-window -lsfml-system
```

---

## What to Watch For

- **Color drift** — populations slowly shift color as mutations accumulate over generations
- **Size pressure** — when food is scarce, smaller creatures tend to survive longer
- **Speed trade-off** — faster creatures find food sooner but die quicker if food runs out
- **Population crashes** — a boom in population can strip the food supply, causing a sudden die-off
