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

## 🔧 Prerequisites & Setup

> **OS**: Windows 10 or later. The build toolchain uses MSYS2 (a Unix-like environment for Windows).

### Step 1 — Install MSYS2

1. Download the installer from [**msys2.org**](https://www.msys2.org/).
2. Run it and install to the default path (`C:\msys64`).
3. When the installer finishes, it opens an MSYS2 terminal. Run this to update the package database:

   ```bash
   pacman -Syu
   ```

   The terminal may close and ask you to reopen it — that's normal. Reopen **"MSYS2 UCRT64"** from the Start Menu and run:

   ```bash
   pacman -Su
   ```

### Step 2 — Install the C++ Compiler (g++)

In the **MSYS2 UCRT64** terminal, install the MinGW-w64 GCC toolchain:

```bash
pacman -S mingw-w64-ucrt-x86_64-toolchain

```

Verify it installed correctly:

```bash
g++ --version
```

You should see something like `g++ (Rev..., Built by MSYS2 project) 13.x.x` or newer.

### Step 3 — Install Raylib

Still in the same MSYS2 UCRT64 terminal:

```bash
pacman -S mingw-w64-ucrt-x86_64-raylib
```

This installs the Raylib headers to `C:\msys64\ucrt64\include` and libraries to `C:\msys64\ucrt64\lib`.

### Step 4 — Add MSYS2 to your System PATH

So that VS Code (and any terminal) can find `g++`:

1. Open **Settings** → search **"Environment Variables"** → click **"Edit the system environment variables"**.
2. Under **System variables**, find `Path` and click **Edit**.
3. Click **New** and add:
   ```
   C:\msys64\ucrt64\bin
   ```
4. Click **OK** on all dialogs and **restart any open terminals / VS Code**.

Verify by opening a **new** PowerShell or CMD and running:

```bash
g++ --version
```

### Step 5 — Install VS Code + C/C++ Extension (recommended)

1. Install [**Visual Studio Code**](https://code.visualstudio.com/) if you don't have it.
2. Open VS Code → go to the **Extensions** tab (`Ctrl+Shift+X`).
3. Search and install **"C/C++"** by Microsoft (provides IntelliSense, debugging, etc.).

---

## 🚀 Building & Running

### Option A: VS Code (recommended)

1. Open the project folder in VS Code (`File → Open Folder`).
2. Press **`Ctrl+Shift+B`** to build — this uses the pre-configured task in `.vscode/tasks.json`.
3. If the build succeeds, you'll see `Evolution.exe` appear in the project directory.
4. Run it by either:
   - Opening a terminal in VS Code (`` Ctrl+` ``) and typing `.\Evolution.exe`
   - Or double-clicking `Evolution.exe` in File Explorer.

### Option B: Command line (MSYS2 UCRT64 terminal)

Navigate to the project folder and run:

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

### ⚠️ Troubleshooting

| Problem | Fix |
|---|---|
| `g++ is not recognized` | You didn't add `C:\msys64\ucrt64\bin` to your system PATH (Step 4), or you need to restart your terminal. |
| `raylib.h: No such file` | Raylib isn't installed. Run `pacman -S mingw-w64-ucrt-x86_64-raylib` in the MSYS2 UCRT64 terminal (Step 3). |
| `undefined reference to ...` | Make sure you're linking all four libraries: `-lraylib -lopengl32 -lgdi32 -lwinmm`. Order matters — keep `-lraylib` first. |
| Window opens then immediately closes | Run from a terminal so you can see any error messages. Check that your GPU drivers are up to date (Raylib uses OpenGL). |

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
