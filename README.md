# 🧬 Darwin — Evolution Simulator

> A real-time **Darwinian natural selection** simulator built with **C++** and **Raylib**.  
> Watch a population of creatures compete for food, reproduce with genetic mutations, survive environmental hazards, and evolve emergent survival strategies — all rendered with glowing, particle-like visuals on a dark canvas.

---

## 📸 Overview

Darwin drops 80 creatures into a bounded world with limited food. Every creature carries a **Genome** — a heritable blueprint encoding body size, speed, vision range, color, and shape. Creatures that eat efficiently live longer, reproduce more, and pass their genes forward. Creatures that starve die. Over generations, natural selection drives the population toward whatever traits best exploit the current environment.

You can watch passively, or intervene: call in meteor strikes, trigger volcanic eruptions, or detonate a nuke. After a disaster, survivors carry **immunity badges** that protect their lineage from the same hazard.

No strategy is hardcoded. Everything that emerges — the average size shrinking, speed settling into a band, vision sweeping upward — is a product of the rules interacting.

---

## ✨ Features

### 🔬 Core Evolution

| Feature | Details |
|---|---|
| **Heritable genome** | Size, speed, vision range, RGB color, body shape — all encoded in `Genome` and passed parent → child |
| **Sexual reproduction** | Two creatures path-find toward each other, meet, and crossover genes (coin-flip per trait) before mutating |
| **Mutation system** | Per-trait mutation chance (default 80%) with bounded random deltas; shape mutation is rarer (8%) |
| **Natural selection** | Energy runs out → creature dies. More food eaten → more reproduction. Traits that help survival propagate |
| **Generations** | A new generation is logged every time the whole population turns over; per-generation statistics are preserved |

### 🧠 Creature AI & Behavior

Each creature runs a priority-ordered decision stack every frame:

1. **Flee** — If inside a hazard, sprint at MAX_SPEED toward a safe exit point (highest priority).
2. **Mate pathfinding** — If paired for reproduction, steer toward the partner.
3. **Food seeking** — Scan within `visionRange`, steer toward nearest food pellet using seek-steering.
4. **Wandering** — No food visible? Apply random velocity nudges and drift.

All movement is bounded by wall bouncing and a configurable `MAX_SPEED` cap.

### ⚡ Mutation Stacks

Beyond genome-level mutations, creatures can acquire **special stacked mutations** at reproduction (5% chance per birth):

| Mutation | Effect | Inheritance |
|---|---|---|
| 🟡 **Speed** | +10–40% speed boost (additive per stack) | 100% — child always inherits parent's best |
| 🔵 **Vision** | +5–15% vision range (additive, capped at +1000%) | 100% — child inherits max parent bonus |
| 🟢 **Stamina** | −5% energy consumption per stack (capped at −50%) | 100% — child inherits max stack count |
| 🟣 **Size** | +10% body size multiplicative per stack (increases HP) | 50% — coin-flip per birth |
| ✨ **Bioluminescence** | Neon color override (green / magenta / cyan / yellow / orange) | 70% — child usually inherits neon glow |

Mutation stack icons are rendered as small overlay symbols directly on each creature's body.

### 🌋 Hazards & Environmental Dangers

Hazards are auto-spawned periodic danger zones with distinct mechanics:

| Hazard | Color | Effect |
|---|---|---|
| **Radiation (RAD)** | 🔴 Red | High instant HP damage per frame while inside zone |
| **Toxic (TOX)** | 🟣 Purple | Lower instant damage + 3-second lingering poison DOT after leaving |
| **Famine (FAM)** | 🟤 Bronze | 2× energy drain for any creature inside (badge holders only suffer 1.25×) |

Survivors of a hazard that would have killed them receive a **permanent immunity badge** (rendered as a small icon):
- `TOX` survivor → immune to future Toxic hazard damage
- `RAD` survivor → immune to future Radiation hazard damage
- `FAM` survivor → receives the bronze hexagon **Famine Badge** (reduced drain)

Immunity is NOT automatically inherited — children must survive the hazard themselves.

### 💥 Player-Triggered Disasters

Disasters are user-controlled events with a **10-second cooldown** between uses. Each has distinct mechanics and a visual countdown:

| Disaster | How to Trigger | Effect |
|---|---|---|
| ☄️ **Meteor Strike** | Click 3 map locations | Three simultaneous impacts, each dealing **70% of max HP** in a 100px radius. 0.5s falling animation + 1.5s impact flash |
| 🌋 **Volcano** | One-click instant | 5-second rumble countdown, then 7 seconds of eruption. Continuous body damage (40 HP/s) + lava puddles scattered around the vent (5 HP/s each, 5s lifespan) |
| ☢️ **Nuke** | Click target location | 5-second countdown with animated radiation rings, then detonation: **instant kill** within 200px inner radius, **80% HP damage** in the 250px outer ring. Full mushroom cloud animation |

### ⏱️ Time Controls

| Mode | Key | Description |
|---|---|---|
| **Play / Pause** | `Space` | Toggle simulation on/off |
| **Fast Forward** | `→` (hold) | Runs at `SimSpeedMult × 2` ticks per frame |
| **Rewind** | `←` (hold) | Restores the last saved snapshot. Up to 600 frames (~10s) of history |
| **Speed Multiplier** | Panel buttons | 1× / 5× / 10× / 15× / 20× real-time speed |
| **Food Drop Rate** | Panel buttons | 1× / 2× / 5× / 10× / 20× food spawn rate |

Rewind uses a **full state snapshot system** — every frame, the complete population, food positions, hazard states, and all counters are serialized to a `deque<Snapshot>`. Rewinding pops and restores them exactly.

A **red VHS-style overlay** flashes on screen during rewind for visual feedback.

### 🎥 Camera System

| Action | Input |
|---|---|
| **Zoom** | Mouse scroll wheel |
| **Pan** | Middle-click drag or right-click drag |
| **Track a creature** | Left-click any creature → camera smoothly follows it at 2.5× zoom |
| **Release tracking** | Scroll, middle-click pan, or right-click |
| **Fullscreen** | `F11` |
| **Resize window** | Drag window edge — simulation pauses, world bounds auto-adjust |

### 🖥️ Live Statistics Panel

The right-side panel displays real-time evolutionary data:

| Stat | Description |
|---|---|
| **Population** | Current living creature count |
| **Peak** | All-time maximum population |
| **Food** | Active food pellets on the field |
| **Births** | Total creatures ever born |
| **Fights** | Total combat encounters |
| **Avg Size** | Mean body radius (with delta vs. previous generation) |
| **Avg Speed** | Mean velocity magnitude (with delta) |
| **Avg Vision** | Mean vision range (with delta) |
| **Generation** | Current generation number |
| **Hazards** | Active hazard zone count |
| **Pop History** | Rolling 120-frame line graph of population |
| **Rewind Buffer** | Visual bar showing how much rewind history is stored |
| **Playback State** | `▶ PLAYING`, `‖ PAUSED`, `▶▶ FAST FWD`, or `◀◀ REWIND` |

Clicking a creature opens an **Inspector panel** showing: ID, age, HP bar, energy bar, genome values, active mutations, immunity badges, and DOT/buff status.

### 💾 File I/O

| Key | Action |
|---|---|
| `S` | Save all creature genomes to `genomes_saved.txt` |
| `L` | Load genomes from `genomes_saved.txt` |
| `G` | Generate a full text report to `simulation_report.txt` |

### 🎨 Visual Design

- **Layered glow** — Creatures rendered with multiple alpha-blended circles for a soft neon bloom effect.
- **Motion trails** — Fading 10-frame position trails behind every creature.
- **Energy ring** — Color-coded ring per creature: 🟢 green (full) → 🟡 yellow → 🔴 red (critical).
- **HP ring** — Secondary ring shows health (relevant when hazards or combat are dealing HP damage separately from energy).
- **Vision ring** — Very faint per-creature circle showing perception radius.
- **Bioluminescence** — Creatures with the bio mutation glow with a bright neon color overriding their genome color.
- **Mutation icons** — Small symbol overlays directly on the creature body: ⚡ speed, 👁 vision, ⚡ stamina, ● size, ■ bio.
- **Dark grid background** — Subtle 50px grid over a near-black `(15,15,20)` canvas.
- **Body shapes** — Five shapes inherited via `Genome.shapeId`: Circle, Triangle, Diamond, Pentagon, Hexagon.
- **Big food** — Rare golden food pellets (10% spawn chance) that grant a temporary **infinite energy buff** when eaten.

---

## 🏗️ Project Structure

```
darwin-evo-sim/
├── main.cpp            # Window, camera, input, game loop, disaster placement
├── Simulation.h/cpp    # Core sim: spawning, physics, eating, reproduction, events, rewind
├── Creature.h/cpp      # Creature AI, movement, rendering, mutation inheritance
├── Genome.h/cpp        # Heritable blueprint, fitness, mutation, file I/O
├── Food.h/cpp          # Food struct, big food, rendering
├── Hazard.h/cpp        # Environmental hazard zones (RAD / TOX / FAM)
├── Disaster.h/cpp      # Player disasters (Meteor / Volcano / Nuke), factory methods
├── Entity.h            # Abstract base class: draw(), getPosition(), getRadius()
└── UI.h/cpp            # Stats panel, pop graph, playback indicators, inspector
```

### Architecture Overview

```
main.cpp
  ├── Window / camera / input handling
  ├── Disaster placement state machine (Normal → PlacingMeteor / PlacingNuke)
  ├── Click-to-select + camera tracking
  ├── calls Simulation::update() × SimSpeedMult  (Playing / FastForward)
  ├── calls Simulation::rewindOneStep()           (Rewinding)
  └── draws grid → sim.draw() → UI overlay

Simulation
  ├── owns m_population  (vector<unique_ptr<Creature>>)
  ├── owns m_food        (vector<Food>)
  ├── owns m_hazards     (vector<unique_ptr<Hazard>>)
  ├── owns m_disasters   (vector<Disaster>, user-triggered)
  ├── owns m_history     (deque<Snapshot>, max 600 frames)
  ├── owns m_events      (priority_queue<SimEvent>, scheduled events)
  ├── update(): snapshot → events → hazards → disasters → AI → eating
  │             → reproduction → death → food regrowth → generation tick
  └── rewindOneStep(): pop snapshot, restore full state

Creature  (inherits Entity)
  ├── update(): DOT → flee → mate → seek food → wander → move → energy burn
  ├── draw(): vision ring → trail → glow → shape → energy/HP ring → mutation icons
  └── reproduce(): crossover → mutate genome → inherit mutation stacks → new roll

Hazard    (inherits Entity)
  ├── update(): decrement lifetime
  └── draw(): pulsing layered glow rings with type label

Disaster
  ├── Factory: createMeteor() / createVolcano() / createNuke()
  ├── update(): state machine Pending → Active → Done
  ├── applyDamage(): per-type damage logic against population
  └── draw(): per-type animation (falling meteor / eruption particles / nuke flash)

Genome
  ├── fitness(): composite score (size efficiency + speed + vision)
  ├── mutate(): per-trait independent mutation with bounded deltas
  ├── operator< / operator== overloading
  └── save() / load(): text-stream file I/O

UI
  ├── update(): rolling population history buffer (120 frames)
  ├── draw(): stat blocks, bars, pop graph, playback state, inspector panel
  └── drawGraph(): line graph rendered from m_popHistory
```

---

## 🧪 How Evolution Emerges

The simulator has no hard-coded "winning" phenotype. Everything emerges from four equations and a selection pressure:

**Energy cost per frame:**
```
cost = (radius × 0.012) + (speed × 0.015) + (visionRange × 0.0003)
cost × staminaReduction × sizeDrainPenalty × famineMult
```

**Reproduction threshold:** `energy ≥ 200` → two compatible creatures meet, crossover, produce offspring.

**Death:** `energy ≤ 0` or `hp ≤ 0` → removed from population.

**Fitness:** `1/(size×0.015+0.01) + speed×0.5 + visionRange×0.01`

What you typically observe:
- **Size** trends downward — smaller bodies burn less energy per frame.
- **Speed** converges to a middle band — too slow means missing food, too fast burns energy quickly.
- **Vision** climbs steadily — wider vision finds food faster, and the cost is relatively small.
- **Color** drifts randomly — neutral trait, pure genetic drift.
- **Post-disaster** — survivors of a lethal hazard carry immunity badges; their descendants are measurably more resilient.
- **Bioluminescent lineages** can take over the population visually while having neutral fitness — a pure color-inheritance showcase.

---

## 📐 Simulation Parameters

| Parameter | Default | Location |
|---|---|---|
| Initial population | 80 | `main.cpp` → `sim.init(80, ...)` |
| Mutation rate | 0.8 (80%) | `main.cpp` → `sim.init(..., 0.8f, ...)` |
| Initial food | 150 | `main.cpp` → `sim.init(..., 150)` |
| Food cap | 200 | `Simulation::update()` |
| Base food spawn chance | 10% per frame | `Simulation::update()` |
| Reproduction energy threshold | 200 | `Simulation::update()` |
| Reproduction cooldown | 300 frames (5s) | `Simulation::update()` |
| Max creature speed | 4.8 | `Creature.h → MAX_SPEED` |
| Steer force | 0.12 | `Creature.h → STEER_FORCE` |
| Trail length | 10 frames | `Creature.h → TRAIL_LEN` |
| Default vision range | 100 px | `Genome.h` |
| Vision range bounds | [20, 300] px | `Genome::mutate()` |
| Rewind buffer | 600 frames (~10s) | `Simulation.h → MAX_HISTORY` |
| Disaster cooldown | 600 frames (10s) | `Disaster.h → COOLDOWN_FRAMES` |
| Meteor damage | 70% of max HP | `Disaster.cpp` |
| Nuke inner kill radius | 200 px | `Disaster.cpp` |
| Nuke outer damage radius | 250 px (80% HP) | `Disaster.cpp` |
| Volcano eruption duration | 420 frames (7s) | `Disaster.cpp` |
| Special mutation chance | 5% per birth | `Creature::reproduce()` |
| Bioluminescence chance | 10% on any mutation | `Creature::reproduce()` |

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
3. If the build succeeds, you'll see `darwin_sim.exe` appear in the project directory.
4. Run it by either:
   - Opening a terminal in VS Code (`` Ctrl+` ``) and typing `.\darwin_sim.exe`
   - Or double-clicking `darwin_sim.exe` in File Explorer.

### Option B: Command line (MSYS2 UCRT64 terminal)

Navigate to the project folder and run:

```bash
g++ -g main.cpp Simulation.cpp Creature.cpp Food.cpp UI.cpp \
    Hazard.cpp Disaster.cpp Genome.cpp \
    -o darwin_sim.exe \
    -I C:/msys64/ucrt64/include \
    -L C:/msys64/ucrt64/lib \
    -lraylib -lopengl32 -lgdi32 -lwinmm
```

Then run:

```bash
./darwin_sim.exe
```

### ⚠️ Troubleshooting

| Problem | Fix |
|---|---|
| `g++ is not recognized` | You didn't add `C:\msys64\ucrt64\bin` to your system PATH (Step 4), or you need to restart your terminal. |
| `raylib.h: No such file` | Raylib isn't installed. Run `pacman -S mingw-w64-ucrt-x86_64-raylib` in the MSYS2 UCRT64 terminal (Step 3). |
| `undefined reference to ...` | Make sure all source files are listed in the compile command. Keep `-lraylib` before `-lopengl32`. |
| Window opens then immediately closes | Run from a terminal so you can see any error messages. Check that your GPU drivers are up to date (Raylib uses OpenGL). |

---

## 🎮 Controls

### Playback

| Key | Action |
|---|---|
| `Space` | Pause / Resume |
| `→` (hold) | Fast Forward |
| `←` (hold) | Rewind through history |
| `F11` | Toggle Fullscreen |
| `Esc` | Cancel disaster placement / Quit |

### Camera

| Input | Action |
|---|---|
| Mouse scroll | Zoom in / out (0.1× – 4×) |
| Middle-click drag | Pan camera |
| Right-click drag | Pan camera |
| Left-click creature | Select + track creature |
| Right-click (world) | Deselect creature |

### Disasters

| Action | Input |
|---|---|
| **Meteor** | Click `METEOR` button → click 3 map locations |
| **Volcano** | Click `VOLCANO` button → placed randomly |
| **Nuke** | Click `NUKE` button → click target location |
| Cancel placement | `Esc` or right-click |

### File I/O

| Key | Action |
|---|---|
| `S` | Save genomes to `genomes_saved.txt` |
| `L` | Load genomes from `genomes_saved.txt` |
| `G` | Generate report to `simulation_report.txt` |

---

## 🧱 OOP & C++ Concepts Demonstrated

This project was built as a showcase of modern C++ and object-oriented design:

| Concept | Where |
|---|---|
| **Inheritance** | `Entity` → `Creature`, `Entity` → `Hazard` |
| **Polymorphism** | `Entity::draw()`, `getPosition()`, `getRadius()` — virtual dispatch drives all rendering |
| **Operator overloading** | `Genome::operator<` (fitness ranking), `Genome::operator==` (genomic match) |
| **Smart pointers** | `vector<unique_ptr<Creature>>`, `vector<unique_ptr<Hazard>>` |
| **STL containers** | `vector`, `deque`, `priority_queue`, `queue` |
| **`std::random`** | `mt19937` engine + typed distributions throughout |
| **File I/O** | `Genome::save()` / `Genome::load()` via `ofstream` / `ifstream` |
| **Event system** | `priority_queue<SimEvent>` — scheduled game events sorted by trigger frame |
| **Template / lambda** | Lambdas for `showStatus`, `remove_if` cleanup, comparators |
| **Factory pattern** | `Disaster::createMeteor()`, `createVolcano()`, `createNuke()` |
| **State machine** | `DisasterState::Pending → Active → Done` per disaster instance |

---

## 📝 License

This project is provided for educational purposes. Feel free to use, modify, and learn from it.
