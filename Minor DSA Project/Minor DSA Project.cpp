/*
 * DAY 2 — Bubble Sort + Keyboard Controls
 * ────────────────────────────────────────
 * Commit message: "feat: add bubble sort step engine, SPACE/R controls, compare/swap colors"
 *
 * What's new vs Day 1:
 *   - SortState struct holds bars + colorMap + step list
 *   - Bubble sort pre-generates all steps as lambdas (step-engine pattern)
 *   - SPACE  → start / pause
 *   - R      → shuffle and reset
 *   - UP/DOWN → speed (1-10)
 *   - Bars turn yellow (comparing) or red (swapping), green when sorted
 *
 * Compile:
 *   g++ -std=c++17 day2_bubble_sort.cpp -o day2 \
 *       $(pkg-config --libs --cflags raylib)
 */

#include "raylib.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <functional>
#include <cmath>

 // ── Layout ───────────────────────────────────────────────
static const int SW = 1600;
static const int SH = 900;
static const int BAR_COUNT = 100;
static const int BAR_GAP = 2;
static const int PANEL_H = 60;   // simple top panel
static const int BOT_PAD = 20;

// ── Colors ───────────────────────────────────────────────
static const Color C_BG = { 10, 12, 20, 255 };
static const Color C_PANEL = { 18, 20, 34, 255 };
static const Color C_TEXT = { 220,224,240,255 };
static const Color C_BAR = { 70, 130,210,255 };
static const Color C_COMPARE = { 255,210, 60,255 };   // yellow  – comparing
static const Color C_SWAP = { 255, 90, 90,255 };   // red     – swapping
static const Color C_SORTED = { 80,230,130,255 };   // green   – sorted

// ── State ────────────────────────────────────────────────
struct SortState {
    std::vector<int> bars;
    std::vector<int> colorMap;   // 0 default · 1 compare · 2 swap · 3 sorted
    bool   running = false;
    bool   finished = false;
    int    speed = 5;         // 1..10
    long long comparisons = 0;
    long long swaps = 0;
    std::vector<std::function<void()>> steps;
    size_t stepIdx = 0;
};

static void resetColors(SortState& s) {
    std::fill(s.colorMap.begin(), s.colorMap.end(), 0);
}

static void shuffle(SortState& s) {
    static std::mt19937 rng(std::random_device{}());
    std::iota(s.bars.begin(), s.bars.end(), 1);
    std::shuffle(s.bars.begin(), s.bars.end(), rng);
    resetColors(s);
    s.running = s.finished = false;
    s.steps.clear();
    s.stepIdx = s.comparisons = s.swaps = 0;
}

// ── Bubble sort: pre-build all steps ─────────────────────
static void buildBubble(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            s.steps.push_back([&s, j, i, n]() {
            resetColors(s);
            s.colorMap[j] = 1; s.colorMap[j + 1] = 1;
            s.comparisons++;
            if (s.bars[j] > s.bars[j + 1]) {
                std::swap(s.bars[j], s.bars[j + 1]);
                s.colorMap[j] = 2; s.colorMap[j + 1] = 2;
                s.swaps++;
            }
            for (int k = n - i - 1; k < n; k++) s.colorMap[k] = 3;
                });
    s.steps.push_back([&s, n]() {
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Draw bars ────────────────────────────────────────────
static void drawBars(const SortState& s) {
    int n = (int)s.bars.size();
    int bw = (SW - BAR_GAP * (n + 1)) / n;
    int areaH = SH - PANEL_H - BOT_PAD;

    for (int i = 0; i < n; i++) {
        float t = (float)s.bars[i] / n;
        int   bh = (int)(t * areaH);
        int   bx = BAR_GAP + i * (bw + BAR_GAP);
        int   by = PANEL_H + (areaH - bh);

        Color c;
        switch (s.colorMap[i]) {
        case 1:  c = C_COMPARE; break;
        case 2:  c = C_SWAP;    break;
        case 3:  c = C_SORTED;  break;
        default: c = C_BAR;     break;
        }
        DrawRectangle(bx, by, bw, bh, c);
    }
}

// ── Draw simple panel ────────────────────────────────────
static void drawPanel(const SortState& s) {
    DrawRectangle(0, 0, SW, PANEL_H, C_PANEL);
    DrawLine(0, PANEL_H, SW, PANEL_H, { 40,44,70,255 });

    const char* status = s.finished ? "SORTED" : s.running ? "RUNNING" : "PAUSED";
    DrawText(TextFormat("Bubble Sort  |  %s  |  Comparisons: %lld  |  Swaps: %lld  |  Speed: %d",
        status, s.comparisons, s.swaps, s.speed),
        16, 10, 18, C_TEXT);
    DrawText("[SPACE] Start/Pause   [R] Shuffle   [UP/DOWN] Speed",
        16, 36, 14, { 120,128,160,255 });
}

int main()
{
    InitWindow(SW, SH, "Sorting Visualizer - Day 2: Bubble Sort");
    SetTargetFPS(60);

    SortState s;
    s.bars.resize(BAR_COUNT);
    s.colorMap.resize(BAR_COUNT, 0);
    shuffle(s);

    while (!WindowShouldClose())
    {
        // ── Input ────────────────────────────────────────
        if (IsKeyPressed(KEY_R)) shuffle(s);

        if (IsKeyPressed(KEY_SPACE)) {
            if (s.finished) { shuffle(s); }
            else {
                if (!s.running && s.steps.empty()) buildBubble(s);
                s.running = !s.running;
            }
        }
        if (IsKeyPressed(KEY_UP))   s.speed = std::min(10, s.speed + 1);
        if (IsKeyPressed(KEY_DOWN)) s.speed = std::max(1, s.speed - 1);

        // ── Advance steps ────────────────────────────────
        if (s.running && !s.finished) {
            int spf = (int)std::round(std::pow(2.8f, (s.speed - 1) / 3.0f));
            for (int k = 0; k < spf && s.stepIdx < s.steps.size(); k++)
                s.steps[s.stepIdx++]();
            if (s.stepIdx >= s.steps.size()) {
                s.running = false; s.finished = true;
                for (auto& c : s.colorMap) c = 3;
            }
        }

        // ── Draw ─────────────────────────────────────────
        BeginDrawing();
        ClearBackground(C_BG);
        drawBars(s);
        drawPanel(s);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}