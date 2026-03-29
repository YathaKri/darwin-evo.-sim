# 🧬 Evolution Simulator

A real-time natural selection simulation built with **C++** and **Raylib**. Creatures roam a 2D world, hunt for food, reproduce with mutation, and die — letting evolution play out live on screen with every run producing something different.

![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Raylib](https://img.shields.io/badge/Raylib-5.5-black?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

---

## ✨ Features

- 🔴 **Energy rings** — color shifts green → yellow → red as a creature's energy drops
- 💫 **Motion trails** — each creature leaves a fading trail showing speed and direction
- 🌟 **Glow effects** — soft layered glow around every creature and food pellet
- 📊 **Live stats panel** — population, peak pop, food count, births, avg size & speed
- 📈 **Population history graph** — watch booms and crashes in real time
- 🌱 **Dynamic food regrowth** — food slowly regenerates, creating natural pressure cycles
- 🔬 **Mutation system** — size, speed, and color all drift across generations

---

## 🧠 How It Works

Every creature has three heritable traits that are passed down and mutated each generation:

| Trait | Effect |
|-------|--------|
| 🔵 **Size** | Bigger creatures eat food more easily but burn energy faster |
| ⚡ **Speed** | Faster creatures cover more ground but cost more energy |
| 🎨 **Color** | Drifts each generation — a visual trace of lineage over time |

Each frame, creatures move around the world bouncing off walls and burning energy. When a creature eats enough food to reach **200 energy** it reproduces, cloning itself with small random mutations. When energy hits **zero** the creature dies.

Food slowly regrows each frame (10% chance per frame, capped at 200 pellets), creating natural boom-and-bust population cycles.

---

## 🗂️ Project Structure

```
Evolution/
├── main.cpp            # Window setup and main loop
├── Simulation.h/.cpp   # Core logic — spawning, eating, death
├── Creature.h/.cpp     # Movement, energy, reproduction & mutation, drawing
├── Food.h/.cpp         # Food spawning and drawing
├── UI.h/.cpp           # Sidebar stats panel and population graph
└── .vscode/
    ├── tasks.json              # Build task
    └── c_cpp_properties.json   # IntelliSense config
```

---

## ⚙️ Requirements

- [MSYS2](https://www.msys2.org/) with the **UCRT64** toolchain
- **Raylib 5.x** installed via MSYS2
- **VS Code** with the C/C++ extension (optional but recommended)

### Install Raylib

Open the MSYS2 UCRT64 terminal and run:

```bash
pacman -S mingw-w64-ucrt-x86_64-raylib
```

---

## 🔨 Building

Open the project folder in VS Code and press `Ctrl+Shift+B` to run the default build task. This compiles all source files and outputs `Evolution.exe`.

Or build manually from the terminal:

```bash
g++ -g main.cpp Simulation.cpp Creature.cpp Food.cpp UI.cpp \
    -o Evolution.exe \
    -I C:/msys64/ucrt64/include \
    -L C:/msys64/ucrt64/lib \
    -lraylib -lopengl32 -lgdi32 -lwinmm
```

---

## 👀 What to Watch For

> The sim is different every run — here's what makes it interesting to observe:

- **Color drift** — populations slowly shift hue as mutations stack up over generations
- **Size pressure** — when food is scarce, smaller creatures survive longer since they burn less energy
- **Speed trade-off** — fast creatures find food first but starve quickly if supply drops
- **Population crashes** — a boom strips the food supply, causing a sudden mass die-off
- **Recovery** — after a crash, survivors with efficient traits rebuild the population

---

## 🗺️ Roadmap

- [ ] Creature vision — sense nearby food and steer toward it
- [ ] Predators — a second species that hunts creatures
- [ ] Sexual reproduction — two parents combine traits
- [ ] Exportable stats — save population data to CSV
- [ ] Shader-based bloom — GPU glow using Raylib render textures

---

## 📄 License

MIT — do whatever you want with it.
