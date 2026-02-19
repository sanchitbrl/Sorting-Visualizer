/*
 * DAY 1 — Window + Static Bar Chart
 * ─────────────────────────────────
 * Commit message: "feat: init project, open window and draw shuffled bar array"
 *
 * What's here:
 *   - Raylib window (1600×900)
 *   - Fixed array of 100 integer values (1..100), shuffled once at startup
 *   - Bars drawn as plain blue rectangles, height proportional to value
 *   - No interaction, no sorting — just proof the pipeline works
 *
 * Compile:
 *   g++ -std=c++17 day1_window_and_bars.cpp -o day1 \
 *       $(pkg-config --libs --cflags raylib)
 */

#include "raylib.h"
#include <vector>
#include <numeric>   // std::iota
#include <algorithm> // std::shuffle
#include <random>

static const int SW = 1600;
static const int SH = 900;
static const int BAR_COUNT = 100;
static const int BAR_GAP = 2;
static const int TOP_PAD = 40;   // small top margin
static const int BOT_PAD = 20;

int main()
{
    // ── Build a shuffled array 1..BAR_COUNT ──────────────
    std::vector<int> bars(BAR_COUNT);
    std::iota(bars.begin(), bars.end(), 1);          // fill 1,2,3...
    std::mt19937 rng(42);                             // fixed seed → reproducible
    std::shuffle(bars.begin(), bars.end(), rng);

    // ── Window ───────────────────────────────────────────
    InitWindow(SW, SH, "Sorting Visualizer - Day 1");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground({ 10, 12, 20, 255 });   // dark background

        // ── Draw bars ────────────────────────────────────
        int drawH = SH - TOP_PAD - BOT_PAD;
        int bw = (SW - BAR_GAP * (BAR_COUNT + 1)) / BAR_COUNT;

        for (int i = 0; i < BAR_COUNT; i++)
        {
            float frac = (float)bars[i] / BAR_COUNT;
            int   bh = (int)(frac * drawH);
            int   bx = BAR_GAP + i * (bw + BAR_GAP);
            int   by = TOP_PAD + (drawH - bh);

            DrawRectangle(bx, by, bw, bh, { 70, 130, 210, 255 });
        }

        // Title
        DrawText("Sorting Visualizer", 20, 8, 22, { 220, 224, 240, 255 });

        EndDrawing();
    }

    CloseWindow();
    return 0;
}