# 🧬 Darwin — Evolution Simulator

A real-time **Darwinian natural selection** simulator built with **C++** and **Raylib**. Watch a population of creatures compete for food, reproduce with genetic mutations, and evolve emergent survival strategies — all rendered with glowing, particle-like visuals on a dark canvas.

---

## ✨ Features

### Evolutionary Mechanics
- **Heritable traits** — Body size, speed, color, and vision range are passed from parent to child with random mutations.
- **Natural selection** — Creatures that eat efficiently survive longer and reproduce more, passing their successful traits forward.
- **Energy system** — Every creature burns energy proportional to its body size, movement speed, and vision range. Eat food to survive, starve to die.
- **Asexual reproduction** — When a creature accumulates enough energy (≥ 200), it splits into parent + mutated offspring.

### Creature Vision & Food Seeking
- Each creature has a **vision range** trait controlling how far it can detect food.
- Creatures **steer toward** the closest visible food pellet using a simple seek-steering behavior.
- A **max speed cap** (4.0) prevents runaway acceleration.
- Vision has a **biological cost** — wider vision drains more energy per frame, creating a natural trade-off between awareness and efficiency.

### Time Manipulation
| Key | Action |
|---|---|
| `Space` | Toggle **Pause** / Play |
| `→` (hold) | **Fast Forward** (5× speed) |
| `←` (hold) | **Rewind** (step backwards through history) |

- **Snapshot-based rewind** — The simulator stores the last **600 frames** (~10 seconds) of full simulation state. Holding left arrow replays them in reverse, restoring creatures, food, and all stats exactly.
- **Fast forward** runs 5 physics updates per render frame for accelerated viewing.
- A subtle **red VHS-style overlay** flashes during rewind for visual feedback.

### Live Statistics Dashboard
A side panel displays real-time evolutionary data:

| Stat | Description |
|---|---|
| Population | Current number of living creatures |
| Peak | Highest population ever reached |
| Food | Number of food pellets on the field |
| Births | Total creatures ever born |
| Avg Size | Mean body radius across the population |
| Avg Speed | Mean velocity magnitude |
| Avg Vision | Mean vision range (the evolving trait!) |
| Pop History | Rolling line graph of population over time |
| Rewind Buffer | Visual bar showing how much rewind history is stored |
| Playback State | Current mode: `>> PLAYING`, `|| PAUSED`, `>>> FAST FWD`, or `<< REWIND` |

### Visual Design
- **Glow effects** — Each creature is rendered with layered alpha-blended circles for a soft neon glow.
- **Motion trails** — Fading trails show recent movement paths.
- **Energy ring** — A color-coded ring around each creature shifts from 🟢 green (full energy) → 🟡 yellow → 🔴 red (starving).
- **Vision debug ring** — A very faint circle shows each creature's perception radius.
- **Food glow** — Food pellets have a soft green bloom effect.
- **Dark grid background** — Subtle grid lines over a near-black canvas.

---

## 🏗️ Project Structure

```
darwin-evo-sim/
├── main.cpp            # Window setup, input handling, game loop
├── Simulation.h/cpp    # Core simulation: spawning, physics, eating, reproduction, rewind
├── Creature.h/cpp      # Creature struct: movement, steering, drawing, mutation
├── Food.h/cpp          # Food struct: spawning and rendering
├── UI.h/cpp            # Stats panel, graphs, playback indicators
└── .vscode/
    └── tasks.json      # VS Code build task (g++ via MSYS2 UCRT64)
```

### Architecture Overview

```
main.cpp
  ├── reads input (Space, ←, →)
  ├── calls Simulation::update() / rewindOneStep()
  ├── calls UI::update() / popLastHistory()
  └── draws everything

Simulation
  ├── owns m_population (vector<Creature>)
  ├── owns m_food (vector<Food>)
  ├── owns m_history (deque<Snapshot>) for rewind
  ├── update(): food regrowth → creature AI → eating → reproduction → death
  └── rewindOneStep(): pops last snapshot, restores full state

Creature
  ├── update(): vision scan → steering → movement → wall bounce → energy burn
  ├── draw(): vision ring → trail → glow → body → energy ring
  └── reproduce(): deep-copy + mutate all traits

Food
  ├── spawn(): random position
  └── draw(): green glow + core
```

---

## 🔧 Prerequisites

- **Compiler**: [MSYS2 UCRT64](https://www.msys2.org/) with `g++` (MinGW-w64)
- **Raylib**: Installed via MSYS2 (`pacman -S mingw-w64-ucrt-x86_64-raylib`)
- **OS**: Windows (the build task uses Windows-specific paths)

---

## 🚀 Building & Running

### Option A: VS Code (recommended)

1. Open the project folder in VS Code.
2. Press `Ctrl+Shift+B` to build (uses the configured task in `.vscode/tasks.json`).
3. Run `Evolution.exe` from the project directory.

### Option B: Command line

```bash
g++ -g main.cpp Simulation.cpp Creature.cpp Food.cpp UI.cpp \
    -o Evolution.exe \
    -I C:/msys64/ucrt64/include \
    -L C:/msys64/ucrt64/lib \
    -lraylib -lopengl32 -lgdi32 -lwinmm
```

Then run:

```bash
./Evolution.exe
```

---

## 🎮 Controls

| Key | Action |
|---|---|
| `Space` | Pause / Resume |
| `→` (hold) | Fast Forward (5× speed) |
| `←` (hold) | Rewind (works while paused too) |
| `Esc` | Quit |

---

## 🧪 How Evolution Emerges

The simulation doesn't have hard-coded "winning strategies." Instead, natural selection emerges from the interplay of simple rules:

1. **Energy formula**: `cost = (radius × 0.015) + (speed × 0.02) + (visionRange × 0.0003)`
2. **Food reward**: +40 energy per pellet eaten.
3. **Reproduction threshold**: 200 energy → split (parent reset to 100, child starts at 100).
4. **Death**: Energy ≤ 0 → removed.

Over time you'll typically observe:
- **Size** tends to shrink (smaller = cheaper to maintain).
- **Speed** settles into a moderate range (too slow = can't reach food, too fast = burns energy).
- **Vision range** finds a sweet spot (wider vision finds food faster but costs more energy).
- **Color drift** is neutral — it's not linked to fitness, so it undergoes random walk / genetic drift.

---

## 📐 Simulation Parameters

| Parameter | Value | Location |
|---|---|---|
| World size | 860 × 640 px | `Simulation.h` |
| Initial population | 30 | `Simulation.cpp` |
| Initial food | 150 | `Simulation.cpp` |
| Food cap | 200 | `Simulation::update()` |
| Food spawn chance | 10% per frame | `Simulation::update()` |
| Reproduce threshold | 200 energy | `Simulation::update()` |
| Max speed | 4.0 | `Creature.h` |
| Steer force | 0.12 | `Creature.h` |
| Default vision range | 100.0 | `Creature.h` |
| Vision range bounds | [20, 300] | `Creature::reproduce()` |
| Rewind buffer | 600 frames (~10s) | `Simulation.h` |
| Fast forward | 5× per frame | `main.cpp` |

---

## 📝 License

This project is provided for educational purposes. Feel free to use, modify, and learn from it.
