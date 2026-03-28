# 🧬 Evolution Simulator

A real-time artificial life simulation built with **C++ and SFML**, where creatures eat, reproduce, mutate, and die — all governed by simple emergent rules. Watch natural selection unfold in your own window.

![C++](https://img.shields.io/badge/C++-17-blue?logo=cplusplus) ![SFML](https://img.shields.io/badge/SFML-3.x-green) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey) ![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 🎬 What It Does

Creatures (colored circles) roam a bounded arena, consuming food (green dots) to survive. When a creature accumulates enough energy, it **reproduces asexually** — passing on its traits to an offspring with small random mutations. Creatures that can't find food in time **die**.

Over time, you'll see the population shift toward traits that happen to be better at finding food in the current environment — emergent evolution with no hand-holding.

### Core Mechanics

| System | Description |
|---|---|
| **Movement** | Creatures move at a fixed velocity and bounce off walls |
| **Energy** | Every frame costs energy — larger and faster creatures burn more |
| **Feeding** | Eating food restores energy; food slowly regrows over time |
| **Reproduction** | Creatures with ≥ 200 energy split into parent + child, each reset to 100 energy |
| **Mutation** | Children inherit parent traits but with random deltas: size, speed, and color all drift |
| **Death** | Any creature hitting 0 energy is removed from the simulation |

---

## 📸 Traits That Evolve

Each creature has three heritable traits that mutate each generation:

- **Size** (radius) — larger creatures have an easier time eating food but burn energy faster
- **Speed** — faster creatures cover more ground but pay a higher energy cost per frame
- **Color** (R/G/B) — a neutral visual marker; useful for watching lineages drift

---

## 🛠️ Prerequisites

- **Windows** (tested on Windows 10/11)
- [MSYS2](https://www.msys2.org/) with the UCRT64 toolchain
- SFML 3.x installed via MSYS2
- Visual Studio Code with the **C/C++ extension** (optional, for the included build task)

### Install SFML via MSYS2

```bash
pacman -S mingw-w64-ucrt-x86_64-sfml
```

---

## 🚀 Building & Running

### Option A — VS Code (recommended)

1. Open the project folder in VS Code.
2. Press **Ctrl+Shift+B** to run the default build task (`Build SFML`).
3. Run the generated `Evolution.exe` from the terminal:

```bash
.\Evolution.exe
```

### Option B — Manual compile

```bash
g++ -g main.cpp -o Evolution.exe \
    -I C:\msys64\ucrt64\include \
    -L C:\msys64\ucrt64\lib \
    -lsfml-graphics -lsfml-window -lsfml-system
```

---

## 📁 Project Structure

```
.
├── main.cpp                  # All simulation logic
├── .vscode/
│   ├── tasks.json            # VS Code build task (g++ + SFML flags)
│   └── c_cpp_properties.json # IntelliSense config for MSYS2/UCRT64
└── README.md
```

---

## ⚙️ Tweakable Parameters

All key constants live at the top of `main.cpp` and are easy to tune:

| Variable / Distribution | Default | Effect |
|---|---|---|
| Initial population | `30` | Starting creature count |
| Initial food | `150` | Starting food count |
| Food regrowth chance | `10%` per frame | How fast the environment recovers |
| Max food cap | `200` | Upper limit on simultaneous food items |
| Energy from food | `+40` | Reward for eating one food item |
| Energy to reproduce | `200` | Threshold that triggers reproduction |
| `mutationDist` | `[-1, 1]` | Size and speed mutation magnitude |
| `colorMutation` | `[-20, 20]` | Per-channel color drift per generation |

---

## 💡 Ideas for Extension

- [ ] Track and display the current generation count
- [ ] Add a population / average-speed graph overlay
- [ ] Introduce predators as a second creature type
- [ ] Add sexual reproduction (two parents combine traits)
- [ ] Serialize a snapshot so you can reload a simulation
- [ ] Add a pause/step-frame mode for observation

---

## 📄 License

MIT — do whatever you like with it.
