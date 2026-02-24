# Sorting Visualizer

A real-time sorting algorithm visualizer built in **C++17** using **[Raylib](https://www.raylib.com/)** as the graphics backend. Watch 6 classic algorithms sort an array of coloured bars step-by-step, with live statistics, animations, and full keyboard control.

---

## Preview

| Element | Description |
|---------|-------------|
| 🔵 Blue bars | Unsorted / default |
| 🟡 Yellow bars | Currently being **compared** |
| 🔴 Red bars | Currently being **swapped** |
| 🟢 Green bars | Confirmed **sorted** |

The UI features a 3-row control panel at the top (title + hints · algorithm buttons · live stats), gradient bars with a glow effect on active elements, subtle grid lines, and a fanfare sweep animation when sorting completes.

---

## Features

- **6 sorting algorithms** — all visualized step by step
- **Live stat cards** — comparisons, swaps, steps, element count
- **Progress bar** — shows how far through the algorithm you are
- **Speed control** — 10 levels, colour-coded green → red
- **Array size selector** — 6 sizes: 25, 50, 75, 100, 150, 200 bars
- **Shuffle wave animation** — bars pop in left-to-right on reset
- **Completion fanfare** — gold highlight sweeps across when sorted
- **Complexity badge** — O(n²) or O(n log n) shown per algorithm
- **FPS counter** in the header
- **MSAA 4× anti-aliasing** + HiDPI window support

---

## Algorithms

| Key | Algorithm | Time Complexity | Space |
|-----|-----------|----------------|-------|
| `1` | Bubble Sort | O(n²) | O(1) |
| `2` | Selection Sort | O(n²) | O(1) |
| `3` | Insertion Sort | O(n²) | O(1) |
| `4` | Merge Sort | O(n log n) | O(n) |
| `5` | Quick Sort | O(n log n) avg | O(log n) |
| `6` | Heap Sort | O(n log n) | O(1) |

---

## Controls

| Key | Action |
|-----|--------|
| `SPACE` | Start / Pause sorting |
| `R` | Shuffle and reset the array |
| `1` – `6` | Select sorting algorithm (auto-shuffles) |
| `UP` | Increase speed |
| `DOWN` | Decrease speed |
| `D` | Increase array size |
| `A` | Decrease array size |

> Pressing `SPACE` after sorting completes will shuffle and reset automatically.

---

## Dependencies

- **C++17** or later
- **[Raylib](https://www.raylib.com/)** 4.x or later

### Installing Raylib

**Ubuntu / Debian**
```bash
sudo apt install libraylib-dev
```

**macOS (Homebrew)**
```bash
brew install raylib
```

**Windows**
Download the MinGW build from [raylib releases](https://github.com/raysan5/raylib/releases) and follow the setup guide.

---

## Building

### Linux / macOS — pkg-config (recommended)
```bash
g++ -std=c++17 day6_final.cpp -o sorting_visualizer \
    $(pkg-config --libs --cflags raylib)
```

### Linux — manual flags
```bash
g++ -std=c++17 day6_final.cpp -o sorting_visualizer \
    -lraylib -lm -lpthread -ldl -lrt -lX11
```

### macOS — manual flags
```bash
g++ -std=c++17 day6_final.cpp -o sorting_visualizer \
    -lraylib -framework OpenGL -framework Cocoa -framework IOKit
```

### Windows (MinGW)
```bash
g++ -std=c++17 day6_final.cpp -o sorting_visualizer.exe \
    -lraylib -lopengl32 -lgdi32 -lwinmm
```

### Run
```bash
./sorting_visualizer        # Linux / macOS
sorting_visualizer.exe      # Windows
```

---

## Project Structure

This project was built incrementally over 6 days, each file a standalone compilable program that builds on the previous one. All files compile identically — just swap the filename.

```
sorting-visualizer/
├── day1_window_and_bars.cpp   # Window + static shuffled bar chart
├── day2_bubble_sort.cpp       # Bubble sort + SPACE/R/speed controls
├── day3_all_algorithms.cpp    # All 6 algorithms + key-based switcher
├── day4_visual_polish.cpp     # Gradient bars, glow, grid lines
├── day5_polished_ui.cpp       # 3-row panel, stat cards, progress bar
├── day6_final.cpp             # Array size selector, animations, fanfare
└── README.md
```

---

## Day-by-Day Commit History

| Day | File | Commit Message |
|-----|------|----------------|
| 1 | `day1_window_and_bars.cpp` | `feat: init project, open window and draw shuffled bar array` |
| 2 | `day2_bubble_sort.cpp` | `feat: add bubble sort step engine, SPACE/R controls, compare/swap colors` |
| 3 | `day3_all_algorithms.cpp` | `feat: add selection/insertion/merge/quick/heap sort + key-based algo switcher` |
| 4 | `day4_visual_polish.cpp` | `feat: gradient bars with rounded caps, glow effect, grid lines` |
| 5 | `day5_polished_ui.cpp` | `feat: redesign UI with 3-row panel, stat cards, progress bar, status pill` |
| 6 | `day6_final.cpp` | `feat: array size selector, shuffle wave animation, sorted fanfare sweep` |

---

## How It Works

The core pattern used throughout is a **step-engine**: before sorting begins, the chosen algorithm pre-generates every comparison and swap as a list of `std::function<void()>` lambdas stored in `SortState::steps`. Each frame, the main loop executes one or more of these lambdas depending on the current speed setting, updating `bars[]` and `colorMap[]` in place. The renderer then draws whatever state those arrays are in.

This approach keeps the sorting logic completely decoupled from the rendering loop — algorithms don't need to know anything about Raylib, and the renderer doesn't need to know anything about sorting.

```
buildSteps()  →  vector<lambda>  →  execute N per frame  →  draw bars[]
```

---

## Configuration

These constants at the top of any file can be tweaked to taste:

| Constant | Default | Description |
|----------|---------|-------------|
| `SW` | `1600` | Window width in pixels |
| `SH` | `900` | Window height in pixels |
| `BAR_GAP` | `2` | Gap between bars in pixels |
| `SIZE_OPTIONS` | `{25,50,75,100,150,200}` | Selectable array sizes |

---

## License

This project is released into the public domain. Do whatever you want with it.

Raylib is licensed under the [zlib license](https://github.com/raysan5/raylib/blob/master/LICENSE).
