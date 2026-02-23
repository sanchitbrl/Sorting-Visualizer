/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  DAY 5 — Full Polished UI                                   ║
 * ║  3-Row Panel · Stat Cards · Progress Bar · Status Pill      ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * Commit: "feat: redesign UI with 3-row panel, stat cards,
 *          progress bar, status pill, and legend overlay"
 *
 * What's new vs Day 4:
 *   - Panel split into three distinct rows:
 *       1. Header bar  — title + complexity badge + status pill
 *       2. Button row  — 6 centered algorithm buttons
 *       3. Stats row   — Comparisons / Swaps / Steps / Elements cards
 *   - Progress bar showing % of steps completed
 *   - Status pill changes colour (yellow=running, green=sorted, grey=paused)
 *   - Complexity badge shown in header  (O(n²) / O(n log n))
 *   - Legend overlay in the bar area
 *   - drawCard() helper for stat tiles
 *   - MSAA 4x anti-aliasing + HiDPI window flags
 *
 * Compile:
 *   g++ -std=c++17 day5_polished_ui.cpp -o day5 \
 *       $(pkg-config --libs --cflags raylib)
 *
 * Controls:
 *   1-6      Select algorithm      SPACE  Start / Pause
 *   R        Shuffle & reset       UP / DOWN  Speed
 */

#include "raylib.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <functional>
#include <cmath>
#include <cstdio>

 // ════════════════════════════════════════════════════════
 //  Layout constants
 // ════════════════════════════════════════════════════════

static const int SW = 1600;
static const int SH = 900;
static const int BAR_COUNT = 100;
static const int BAR_GAP = 2;

static const int HEADER_H = 70;
static const int BTN_ROW_H = 60;
static const int STATS_H = 64;
static const int PANEL_H = HEADER_H + BTN_ROW_H + STATS_H;  // 194
static const int BOT_PAD = 18;

static const int BAR_AREA_Y = PANEL_H;
static const int BAR_AREA_H = SH - PANEL_H - BOT_PAD;

// ════════════════════════════════════════════════════════
//  Colour palette
// ════════════════════════════════════════════════════════

static const Color C_BG = { 10,  12,  20, 255 };
static const Color C_HEADER = { 18,  20,  34, 255 };
static const Color C_PANEL = { 20,  22,  38, 255 };
static const Color C_DIVIDER = { 40,  44,  70, 255 };
static const Color C_TEXT = { 220, 224, 240, 255 };
static const Color C_SUBTEXT = { 120, 128, 160, 255 };
static const Color C_ACCENT = { 99, 155, 255, 255 };
static const Color C_BTN = { 32,  36,  58, 255 };
static const Color C_CARD = { 26,  30,  50, 255 };

// Bar gradient pairs  (lo = bottom colour, hi = top colour)
static const Color C_BAR_LO = { 55, 100, 200, 255 };
static const Color C_BAR_HI = { 90, 160, 255, 255 };
static const Color C_CMP_LO = { 200, 150,  20, 255 };
static const Color C_CMP_HI = { 255, 210,  60, 255 };
static const Color C_SWP_LO = { 200,  40,  40, 255 };
static const Color C_SWP_HI = { 255,  90,  90, 255 };
static const Color C_SRT_LO = { 30, 160,  80, 255 };
static const Color C_SRT_HI = { 80, 230, 130, 255 };

// ════════════════════════════════════════════════════════
//  Algorithm metadata
// ════════════════════════════════════════════════════════

enum Algorithm {
    BUBBLE = 0,
    SELECTION,
    INSERTION,
    MERGE,
    QUICK,
    HEAP,
    ALGO_COUNT
};

static const char* ALGO_NAMES[ALGO_COUNT] = {
    "Bubble Sort",
    "Selection Sort",
    "Insertion Sort",
    "Merge Sort",
    "Quick Sort",
    "Heap Sort"
};

static const char* ALGO_CMPLX[ALGO_COUNT] = {
    "O(n²)", "O(n²)", "O(n²)",
    "O(n log n)", "O(n log n)", "O(n log n)"
};

// ════════════════════════════════════════════════════════
//  Shared state
// ════════════════════════════════════════════════════════

struct SortState {
    std::vector<int> bars;
    std::vector<int> colorMap;   // 0 default · 1 compare · 2 swap · 3 sorted

    Algorithm  algo = BUBBLE;
    bool       running = false;
    bool       finished = false;
    int        speed = 5;      // 1 (slow) … 10 (fast)
    long long  comparisons = 0;
    long long  swaps = 0;

    std::vector<std::function<void()>> steps;
    size_t stepIdx = 0;
};

// ════════════════════════════════════════════════════════
//  Utility helpers
// ════════════════════════════════════════════════════════

static void resetColors(SortState& s)
{
    std::fill(s.colorMap.begin(), s.colorMap.end(), 0);
}

static void shuffle(SortState& s)
{
    static std::mt19937 rng(std::random_device{}());
    std::iota(s.bars.begin(), s.bars.end(), 1);
    std::shuffle(s.bars.begin(), s.bars.end(), rng);
    resetColors(s);
    s.running = false;
    s.finished = false;
    s.comparisons = 0;
    s.swaps = 0;
    s.steps.clear();
    s.stepIdx = 0;
}

// Linear colour interpolation
static Color lerpCol(Color a, Color b, float t)
{
    return {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}

// ════════════════════════════════════════════════════════
//  Sort builders — each pre-generates all steps as lambdas
// ════════════════════════════════════════════════════════

// ── Bubble Sort ─────────────────────────────────────────
static void buildBubble(SortState& s)
{
    int n = (int)s.bars.size();

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            s.steps.push_back([&s, j, i, n]() {
                resetColors(s);
                s.colorMap[j] = 1;
                s.colorMap[j + 1] = 1;
                s.comparisons++;

                if (s.bars[j] > s.bars[j + 1]) {
                    std::swap(s.bars[j], s.bars[j + 1]);
                    s.colorMap[j] = 2;
                    s.colorMap[j + 1] = 2;
                    s.swaps++;
                }

                // Mark sorted tail
                for (int k = n - i - 1; k < n; k++)
                    s.colorMap[k] = 3;
                });
        }
    }

    s.steps.push_back([&s, n]() {
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Selection Sort ──────────────────────────────────────
static void buildSelection(SortState& s)
{
    int n = (int)s.bars.size();

    for (int i = 0; i < n - 1; i++) {
        s.steps.push_back([&s, i, n]() mutable {
            resetColors(s);
            int mi = i;
            s.colorMap[i] = 2;

            for (int j = i + 1; j < n; j++) {
                s.comparisons++;
                s.colorMap[j] = 1;
                if (s.bars[j] < s.bars[mi]) {
                    if (mi != i) s.colorMap[mi] = 0;
                    mi = j;
                    s.colorMap[mi] = 2;
                }
            }

            if (mi != i) {
                std::swap(s.bars[i], s.bars[mi]);
                s.swaps++;
            }

            for (int k = 0; k <= i; k++) s.colorMap[k] = 3;
            });
    }

    s.steps.push_back([&s, n]() {
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Insertion Sort ──────────────────────────────────────
static void buildInsertion(SortState& s)
{
    int n = (int)s.bars.size();

    for (int i = 1; i < n; i++) {
        s.steps.push_back([&s, i]() {
            resetColors(s);
            int key = s.bars[i];
            int j = i - 1;
            s.colorMap[i] = 2;

            while (j >= 0 && s.bars[j] > key) {
                s.bars[j + 1] = s.bars[j];
                s.colorMap[j + 1] = 1;
                j--;
                s.comparisons++;
                s.swaps++;
            }

            s.bars[j + 1] = key;
            s.colorMap[j + 1] = 2;
            });
    }

    s.steps.push_back([&s, n]() {
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Merge Sort (iterative, bottom-up) ───────────────────
static void buildMerge(SortState& s)
{
    int n = (int)s.bars.size();

    for (int w = 1; w < n; w *= 2) {
        for (int i = 0; i < n; i += 2 * w) {
            int l = i;
            int m = std::min(i + w - 1, n - 1);
            int r = std::min(i + 2 * w - 1, n - 1);
            if (m >= r) continue;

            s.steps.push_back([&s, l, m, r]() {
                resetColors(s);
                std::vector<int> tmp(s.bars.begin() + l,
                    s.bars.begin() + r + 1);
                int i2 = 0;
                int j2 = m - l + 1;
                int k = l;

                while (i2 <= m - l && j2 <= r - l) {
                    s.comparisons++;
                    if (tmp[i2] <= tmp[j2])
                        s.bars[k++] = tmp[i2++];
                    else {
                        s.bars[k++] = tmp[j2++];
                        s.swaps++;
                    }
                }

                while (i2 <= m - l) s.bars[k++] = tmp[i2++];
                while (j2 <= r - l) s.bars[k++] = tmp[j2++];

                for (int x = l; x <= r; x++) s.colorMap[x] = 2;
                });
        }
    }

    s.steps.push_back([&s, n]() {
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Quick Sort (iterative) ──────────────────────────────
static void buildQuick(SortState& s)
{
    int n = (int)s.bars.size();

    struct Range { int l, r; };

    std::vector<int>   arr = s.bars;
    std::vector<Range> work;
    work.push_back({ 0, n - 1 });

    while (!work.empty()) {
        Range wr = work.back();
        work.pop_back();

        int l = wr.l;
        int r = wr.r;
        if (l >= r) continue;

        int pivot = arr[r];
        int i2 = l - 1;
        std::vector<int> snap = arr;
        int sL = l, sR = r;

        s.steps.push_back([&s, snap, sL, sR]() mutable {
            resetColors(s);
            int p2 = s.bars[sR];
            int i3 = sL - 1;
            s.colorMap[sR] = 2;

            for (int j = sL; j < sR; j++) {
                s.comparisons++;
                s.colorMap[j] = 1;
                if (s.bars[j] <= p2) {
                    i3++;
                    std::swap(s.bars[i3], s.bars[j]);
                    s.colorMap[i3] = 2;
                    s.swaps++;
                }
            }

            std::swap(s.bars[i3 + 1], s.bars[sR]);
            s.colorMap[i3 + 1] = 3;
            });

        for (int j = l; j < r; j++) {
            if (arr[j] <= pivot) {
                i2++;
                std::swap(arr[i2], arr[j]);
            }
        }

        std::swap(arr[i2 + 1], arr[r]);
        int p = i2 + 1;

        if (p - 1 > l) work.push_back({ l,     p - 1 });
        if (p + 1 < r) work.push_back({ p + 1, r });
    }

    s.steps.push_back([&s, n]() {
        std::sort(s.bars.begin(), s.bars.end());
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Heap Sort ───────────────────────────────────────────
static void heapify(
    std::vector<int>& arr,
    std::vector<std::function<void()>>& steps,
    std::vector<int>& cm,
    long long& cc, long long& sc,
    int n, int i)
{
    int lg = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;

    if (l < n) { cc++; if (arr[l] > arr[lg]) lg = l; }
    if (r < n) { cc++; if (arr[r] > arr[lg]) lg = r; }

    if (lg != i) {
        std::swap(arr[i], arr[lg]);
        sc++;
        int ci = i, cl = lg;
        steps.push_back([&cm, ci, cl]() {
            std::fill(cm.begin(), cm.end(), 0);
            cm[ci] = 2;
            cm[cl] = 1;
            });
        heapify(arr, steps, cm, cc, sc, n, lg);
    }
}

static void buildHeap(SortState& s)
{
    int n = (int)s.bars.size();
    std::vector<int> arr = s.bars;

    // Build max-heap
    for (int i = n / 2 - 1; i >= 0; i--) {
        heapify(arr, s.steps, s.colorMap,
            s.comparisons, s.swaps, n, i);
        std::vector<int> snap = arr;
        s.steps.push_back([&s, snap]() {
            s.bars = snap;
            resetColors(s);
            });
    }

    // Extract elements one by one
    for (int i = n - 1; i > 0; i--) {
        std::swap(arr[0], arr[i]);
        s.swaps++;

        std::vector<int> snap = arr;
        int ss = i;
        s.steps.push_back([&s, snap, ss, n]() {
            s.bars = snap;
            resetColors(s);
            s.colorMap[0] = 2;
            for (int k = ss; k < n; k++) s.colorMap[k] = 3;
            });

        heapify(arr, s.steps, s.colorMap,
            s.comparisons, s.swaps, i, 0);

        std::vector<int> snap2 = arr;
        int si2 = i;
        s.steps.push_back([&s, snap2, si2, n]() {
            s.bars = snap2;
            resetColors(s);
            for (int k = si2; k < n; k++) s.colorMap[k] = 3;
            });
    }

    s.steps.push_back([&s, n]() {
        std::sort(s.bars.begin(), s.bars.end());
        for (int k = 0; k < n; k++) s.colorMap[k] = 3;
        });
}

// ── Dispatcher ──────────────────────────────────────────
static void buildSteps(SortState& s)
{
    s.steps.clear();
    s.stepIdx = 0;
    s.comparisons = 0;
    s.swaps = 0;
    resetColors(s);

    switch (s.algo) {
    case BUBBLE:    buildBubble(s);    break;
    case SELECTION: buildSelection(s); break;
    case INSERTION: buildInsertion(s); break;
    case MERGE:     buildMerge(s);     break;
    case QUICK:     buildQuick(s);     break;
    case HEAP:      buildHeap(s);      break;
    default: break;
    }
}

// ════════════════════════════════════════════════════════
//  Drawing helpers
// ════════════════════════════════════════════════════════

// Gradient bar: top is bright, bottom is dark; rounded bright cap on tip
static void drawGradBar(int x, int y, int w, int h,
    Color lo, Color hi)
{
    if (h <= 0) return;
    DrawRectangleGradientV(x, y, w, h, hi, lo);

    int capH = std::min(h, 5);
    DrawRectangleRounded(
        { (float)x, (float)y, (float)w, (float)capH },
        0.5f, 4, hi
    );
}

// Soft ambient halo behind an active bar
static void drawGlow(int x, int y, int w, int h, Color c)
{
    DrawRectangle(x - 2, y - 3, w + 4, h + 3,
        { c.r, c.g, c.b, 35 });
}

// Stat card with a coloured accent stripe at the top
static void drawCard(int x, int y, int w, int h,
    const char* label, const char* val,
    Color accent)
{
    DrawRectangleRounded(
        { (float)x, (float)y, (float)w, (float)h },
        0.18f, 8, C_CARD
    );
    DrawRectangleRounded(
        { (float)x, (float)y, (float)w, 3.f },
        0.3f, 4, accent
    );
    DrawText(label, x + 12, y + 9, 13, C_SUBTEXT);
    DrawText(val, x + 12, y + 28, 20, C_TEXT);
}

// Outlined rounded rectangle (no fill)
static void drawRoundBorder(float x, float y, float w, float h,
    float radius, Color c)
{
    DrawRectangleRoundedLines({ x, y, w, h }, radius, 8, c);
}

// ════════════════════════════════════════════════════════
//  Bar area
// ════════════════════════════════════════════════════════

static void drawBars(const SortState& s)
{
    int n = (int)s.bars.size();
    int bw = (SW - BAR_GAP * (n + 1)) / n;

    // Subtle horizontal grid lines at 25 / 50 / 75 %
    for (int pct = 25; pct < 100; pct += 25) {
        int gy = BAR_AREA_Y + (int)(BAR_AREA_H * (1.f - pct / 100.f));
        DrawLine(0, gy, SW, gy, { 30, 36, 60, 255 });
        DrawText(TextFormat("%d%%", pct),
            5, gy - 13, 11, { 46, 54, 86, 255 });
    }

    for (int i = 0; i < n; i++) {
        float t = (float)s.bars[i] / n;
        int   bh = (int)(t * BAR_AREA_H);
        int   bx = BAR_GAP + i * (bw + BAR_GAP);
        int   by = BAR_AREA_Y + (BAR_AREA_H - bh);

        Color lo, hi;
        switch (s.colorMap[i]) {
        case 1:
            lo = C_CMP_LO; hi = C_CMP_HI;
            drawGlow(bx, by, bw, bh, hi);
            break;
        case 2:
            lo = C_SWP_LO; hi = C_SWP_HI;
            drawGlow(bx, by, bw, bh, hi);
            break;
        case 3:
            lo = C_SRT_LO; hi = C_SRT_HI;
            break;
        default:
            lo = C_BAR_LO; hi = C_BAR_HI;
            break;
        }

        drawGradBar(bx, by, bw, bh, lo, hi);
    }
}

// ════════════════════════════════════════════════════════
//  UI panels
// ════════════════════════════════════════════════════════

static void drawHeader(const SortState& s)
{
    DrawRectangle(0, 0, SW, HEADER_H, C_HEADER);
    DrawLine(0, HEADER_H, SW, HEADER_H, C_DIVIDER);

    // Title
    DrawText("SORTING VISUALIZER", 28, 20, 28, C_TEXT);

    // Complexity badge
    const char* cx = ALGO_CMPLX[s.algo];
    int         cxW = MeasureText(cx, 17) + 20;
    DrawRectangleRounded(
        { (float)(SW - cxW - 190), 15.f, (float)cxW, 38.f },
        0.4f, 6, { 45, 55, 105, 255 }
    );
    DrawText(cx, SW - cxW - 180, 24, 17, C_ACCENT);

    // Status pill
    const char* label = s.finished ? "SORTED"
        : s.running ? "RUNNING"
        : "PAUSED";
    Color pc = s.finished ? C_SRT_HI
        : s.running ? C_CMP_HI
        : C_SUBTEXT;

    int psW = MeasureText(label, 15) + 24;
    int psX = SW - psW - 18;
    DrawRectangleRounded(
        { (float)psX, 18.f, (float)psW, 32.f },
        0.5f, 6, { pc.r, pc.g, pc.b, 45 }
    );
    drawRoundBorder((float)psX, 18.f, (float)psW, 32.f,
        0.5f, { pc.r, pc.g, pc.b, 190 });
    DrawText(label, SW - psW - 6, 26, 15, pc);
}

static void drawButtonRow(const SortState& s)
{
    DrawRectangle(0, HEADER_H, SW, BTN_ROW_H, C_PANEL);
    DrawLine(0, HEADER_H + BTN_ROW_H, SW, HEADER_H + BTN_ROW_H,
        C_DIVIDER);

    const int btnW = 158;
    const int btnH = 40;
    const int btnGap = 10;
    const int startX = (SW - (ALGO_COUNT * btnW + (ALGO_COUNT - 1) * btnGap)) / 2;

    for (int i = 0; i < ALGO_COUNT; i++) {
        int  bx = startX + i * (btnW + btnGap);
        int  by = HEADER_H + (BTN_ROW_H - btnH) / 2;
        bool active = (s.algo == i);

        Color bg = active ? C_ACCENT : C_BTN;
        Color tc = active ? C_BG : C_TEXT;

        DrawRectangleRounded(
            { (float)bx, (float)by, (float)btnW, (float)btnH },
            0.22f, 8, bg
        );
        if (!active) {
            drawRoundBorder((float)bx, (float)by,
                (float)btnW, (float)btnH,
                0.22f, C_DIVIDER);
        }

        char lbl[48];
        snprintf(lbl, sizeof(lbl), "[%d] %s", i + 1, ALGO_NAMES[i]);
        int tw = MeasureText(lbl, 14);
        DrawText(lbl,
            bx + (btnW - tw) / 2,
            by + (btnH - 14) / 2,
            14, tc);
    }
}

static void drawStatsRow(const SortState& s)
{
    int sY = HEADER_H + BTN_ROW_H;
    DrawRectangle(0, sY, SW, STATS_H, C_PANEL);
    DrawLine(0, sY + STATS_H, SW, sY + STATS_H, C_DIVIDER);

    // ── Stat cards ───────────────────────────────────────
    const int cW = 190;
    const int cH = 50;
    const int cGap = 10;
    const int cX = 18;
    const int cY = sY + 7;

    char buf[64];

    snprintf(buf, sizeof(buf), "%lld", s.comparisons);
    drawCard(cX + 0 * (cW + cGap), cY, cW, cH,
        "Comparisons", buf, C_CMP_HI);

    snprintf(buf, sizeof(buf), "%lld", s.swaps);
    drawCard(cX + 1 * (cW + cGap), cY, cW, cH,
        "Swaps", buf, C_SWP_HI);

    snprintf(buf, sizeof(buf), "%zu / %zu",
        s.stepIdx, s.steps.size());
    drawCard(cX + 2 * (cW + cGap), cY, cW, cH,
        "Steps", buf, C_ACCENT);

    snprintf(buf, sizeof(buf), "%d", BAR_COUNT);
    drawCard(cX + 3 * (cW + cGap), cY, cW, cH,
        "Elements", buf, C_SRT_HI);

    // ── Progress bar ─────────────────────────────────────
    int   pbX = cX + 4 * (cW + cGap) + 8;
    int   pbY = sY + 10;
    int   pbW = 260;
    int   pbH = 10;
    float prog = s.steps.empty()
        ? 0.f
        : (float)s.stepIdx / (float)s.steps.size();

    DrawRectangleRounded(
        { (float)pbX, (float)pbY, (float)pbW, (float)pbH },
        0.5f, 6, C_BTN
    );
    if (prog > 0.f) {
        DrawRectangleRounded(
            { (float)pbX, (float)pbY, pbW * prog, (float)pbH },
            0.5f, 6, C_ACCENT
        );
    }
    DrawText("Progress", pbX, sY + 26, 12, C_SUBTEXT);

    // ── Speed bar ────────────────────────────────────────
    int spX = SW - 290;
    int spY = sY + 8;
    DrawText("Speed", spX, spY, 13, C_SUBTEXT);

    int   sBX = spX + 58;
    int   sBY = spY + 1;
    int   sBW = 160;
    int   sBH = 12;
    float sf = (s.speed - 1) / 9.f;
    Color sc = lerpCol({ 60, 200, 100, 255 }, { 255, 90, 50, 255 }, sf);

    DrawRectangleRounded(
        { (float)sBX, (float)sBY, (float)sBW, (float)sBH },
        0.5f, 6, C_BTN
    );
    if (sf > 0.f) {
        DrawRectangleRounded(
            { (float)sBX, (float)sBY, sBW * sf, (float)sBH },
            0.5f, 6, sc
        );
    }
    DrawText(TextFormat("%d", s.speed),
        sBX + sBW + 8, spY, 18, C_TEXT);

    // ── Keyboard hints ───────────────────────────────────
    DrawText(
        "[SPACE] Start/Pause   [R] Shuffle"
        "   [UP/DOWN] Speed   [1-6] Algorithm",
        16, sY + 44, 13, C_SUBTEXT
    );
}

static void drawLegend()
{
    int lx = SW - 210;
    int ly = BAR_AREA_Y + 14;

    DrawRectangleRounded(
        { (float)(lx - 10), (float)(ly - 8), 210.f, 90.f },
        0.12f, 6, { 8, 10, 18, 190 }
    );

    struct Entry { Color c; const char* label; };
    Entry entries[] = {
        { C_BAR_HI, "Default"   },
        { C_CMP_HI, "Comparing" },
        { C_SWP_HI, "Swapping"  },
        { C_SRT_HI, "Sorted"    },
    };

    for (int i = 0; i < 4; i++) {
        DrawRectangleRounded(
            { (float)lx, (float)(ly + i * 20), 14.f, 12.f },
            0.35f, 4, entries[i].c
        );
        DrawText(entries[i].label,
            lx + 20, ly + i * 20, 14, C_TEXT);
    }
}

static void drawUI(const SortState& s)
{
    drawHeader(s);
    drawButtonRow(s);
    drawStatsRow(s);
    drawLegend();
}

// ════════════════════════════════════════════════════════
//  Main
// ════════════════════════════════════════════════════════

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SW, SH, "Sorting Visualizer — Day 5");
    SetTargetFPS(60);

    SortState s;
    s.bars.resize(BAR_COUNT);
    s.colorMap.resize(BAR_COUNT, 0);
    shuffle(s);

    while (!WindowShouldClose())
    {
        // ── Input ────────────────────────────────────────
        for (int i = 0; i < ALGO_COUNT; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                s.algo = (Algorithm)i;
                shuffle(s);
            }
        }

        if (IsKeyPressed(KEY_R)) {
            shuffle(s);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            if (s.finished) {
                shuffle(s);
            }
            else {
                if (!s.running && s.steps.empty())
                    buildSteps(s);
                s.running = !s.running;
            }
        }

        if (IsKeyPressed(KEY_UP))
            s.speed = std::min(10, s.speed + 1);
        if (IsKeyPressed(KEY_DOWN))
            s.speed = std::max(1, s.speed - 1);

        // ── Advance sort steps ───────────────────────────
        if (s.running && !s.finished) {
            int spf = (int)std::round(
                std::pow(2.8f, (s.speed - 1) / 3.0f)
            );
            for (int k = 0; k < spf && s.stepIdx < s.steps.size(); k++)
                s.steps[s.stepIdx++]();

            if (s.stepIdx >= s.steps.size()) {
                s.running = false;
                s.finished = true;
                for (auto& c : s.colorMap) c = 3;
            }
        }

        // ── Draw ─────────────────────────────────────────
        BeginDrawing();
        ClearBackground(C_BG);
        drawBars(s);
        drawUI(s);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}