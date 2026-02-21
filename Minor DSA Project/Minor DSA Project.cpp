#include "raylib.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <functional>
#include <cmath>
#include <cstdio>

 // ── Layout ───────────────────────────────────────────────
static const int SW = 1600;
static const int SH = 900;
static const int BAR_COUNT = 100;
static const int BAR_GAP = 2;
static const int PANEL_H = 80;
static const int BOT_PAD = 20;

// ── Colors ───────────────────────────────────────────────
static const Color C_BG = { 10, 12, 20, 255 };
static const Color C_PANEL = { 18, 20, 34, 255 };
static const Color C_TEXT = { 220,224,240,255 };
static const Color C_SUB = { 120,128,160,255 };
static const Color C_BAR = { 70, 130,210,255 };
static const Color C_COMPARE = { 255,210, 60,255 };
static const Color C_SWAP = { 255, 90, 90,255 };
static const Color C_SORTED = { 80,230,130,255 };
static const Color C_ACCENT = { 99,155,255,255 };
static const Color C_BTN = { 32,  36, 58,255 };

// ── Algorithms ───────────────────────────────────────────
enum Algorithm { BUBBLE = 0, SELECTION, INSERTION, MERGE, QUICK, HEAP, ALGO_COUNT };

static const char* ALGO_NAMES[] = {
    "Bubble Sort","Selection Sort","Insertion Sort",
    "Merge Sort","Quick Sort","Heap Sort"
};
static const char* ALGO_CMPLX[] = {
    "O(n²)","O(n²)","O(n²)","O(n log n)","O(n log n)","O(n log n)"
};

// ── State ────────────────────────────────────────────────
struct SortState {
    std::vector<int> bars, colorMap;
    Algorithm  algo = BUBBLE;
    bool       running = false;
    bool       finished = false;
    int        speed = 5;
    long long  comparisons = 0, swaps = 0;
    std::vector<std::function<void()>> steps;
    size_t     stepIdx = 0;
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
    s.steps.clear(); s.stepIdx = 0;
    s.comparisons = s.swaps = 0;
}

// ── Sort builders ────────────────────────────────────────
static void buildBubble(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            s.steps.push_back([&s, j, i, n]() {
            resetColors(s);
            s.colorMap[j] = 1; s.colorMap[j + 1] = 1; s.comparisons++;
            if (s.bars[j] > s.bars[j + 1]) {
                std::swap(s.bars[j], s.bars[j + 1]);
                s.colorMap[j] = 2; s.colorMap[j + 1] = 2; s.swaps++;
            }
            for (int k = n - i - 1;k < n;k++) s.colorMap[k] = 3;
                });
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}

static void buildSelection(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 0; i < n - 1; i++)
        s.steps.push_back([&s, i, n]() mutable {
        resetColors(s); int mi = i; s.colorMap[i] = 2;
        for (int j = i + 1;j < n;j++) {
            s.comparisons++; s.colorMap[j] = 1;
            if (s.bars[j] < s.bars[mi]) {
                if (mi != i) s.colorMap[mi] = 0;
                mi = j; s.colorMap[mi] = 2;
            }
        }
        if (mi != i) { std::swap(s.bars[i], s.bars[mi]);s.swaps++; }
        for (int k = 0;k <= i;k++) s.colorMap[k] = 3;
            });
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}

static void buildInsertion(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 1; i < n; i++)
        s.steps.push_back([&s, i]() {
        resetColors(s); int key = s.bars[i], j = i - 1; s.colorMap[i] = 2;
        while (j >= 0 && s.bars[j] > key) {
            s.bars[j + 1] = s.bars[j]; s.colorMap[j + 1] = 1; j--;
            s.comparisons++; s.swaps++;
        }
        s.bars[j + 1] = key; s.colorMap[j + 1] = 2;
            });
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}

static void buildMerge(SortState& s) {
    int n = (int)s.bars.size();
    for (int w = 1; w < n; w *= 2)
        for (int i = 0; i < n; i += 2 * w) {
            int l = i, m = std::min(i + w - 1, n - 1), r = std::min(i + 2 * w - 1, n - 1);
            if (m >= r) continue;
            s.steps.push_back([&s, l, m, r]() {
                resetColors(s);
                std::vector<int> tmp(s.bars.begin() + l, s.bars.begin() + r + 1);
                int i2 = 0, j2 = m - l + 1, k = l;
                while (i2 <= m - l && j2 <= r - l) {
                    s.comparisons++;
                    if (tmp[i2] <= tmp[j2]) s.bars[k++] = tmp[i2++];
                    else { s.bars[k++] = tmp[j2++];s.swaps++; }
                }
                while (i2 <= m - l) s.bars[k++] = tmp[i2++];
                while (j2 <= r - l) s.bars[k++] = tmp[j2++];
                for (int x = l;x <= r;x++) s.colorMap[x] = 2;
                });
        }
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}

static void buildQuick(SortState& s) {
    int n = (int)s.bars.size();
    struct Range { int l, r; };
    std::vector<int> arr = s.bars;
    std::vector<Range> work; work.push_back({ 0,n - 1 });
    while (!work.empty()) {
        Range wr = work.back(); work.pop_back();
        int l = wr.l, r = wr.r;
        if (l >= r) continue;
        int pivot = arr[r], i2 = l - 1;
        std::vector<int> snap = arr; int sL = l, sR = r;
        s.steps.push_back([&s, snap, sL, sR]() mutable {
            resetColors(s);
            int p2 = s.bars[sR], i3 = sL - 1; s.colorMap[sR] = 2;
            for (int j = sL;j < sR;j++) {
                s.comparisons++; s.colorMap[j] = 1;
                if (s.bars[j] <= p2) { i3++;std::swap(s.bars[i3], s.bars[j]);s.colorMap[i3] = 2;s.swaps++; }
            }
            std::swap(s.bars[i3 + 1], s.bars[sR]); s.colorMap[i3 + 1] = 3;
            });
        for (int j = l;j < r;j++) if (arr[j] <= pivot) { i2++;std::swap(arr[i2], arr[j]); }
        std::swap(arr[i2 + 1], arr[r]); int p = i2 + 1;
        if (p - 1 > l) work.push_back({ l,p - 1 });
        if (p + 1 < r) work.push_back({ p + 1,r });
    }
    s.steps.push_back([&s, n]() {
        std::sort(s.bars.begin(), s.bars.end());
        for (int k = 0;k < n;k++) s.colorMap[k] = 3;
        });
}

static void heapify(std::vector<int>& arr,
    std::vector<std::function<void()>>& steps,
    std::vector<int>& cm, long long& cc, long long& sc,
    int n, int i) {
    int lg = i, l = 2 * i + 1, r = 2 * i + 2;
    if (l < n) { cc++;if (arr[l] > arr[lg])lg = l; }
    if (r < n) { cc++;if (arr[r] > arr[lg])lg = r; }
    if (lg != i) {
        std::swap(arr[i], arr[lg]); sc++;
        int ci = i, cl = lg;
        steps.push_back([&cm, ci, cl]() {
            std::fill(cm.begin(), cm.end(), 0); cm[ci] = 2; cm[cl] = 1;
            });
        heapify(arr, steps, cm, cc, sc, n, lg);
    }
}

static void buildHeap(SortState& s) {
    int n = (int)s.bars.size(); std::vector<int> arr = s.bars;
    for (int i = n / 2 - 1;i >= 0;i--) {
        heapify(arr, s.steps, s.colorMap, s.comparisons, s.swaps, n, i);
        std::vector<int> snap = arr;
        s.steps.push_back([&s, snap]() { s.bars = snap; resetColors(s); });
    }
    for (int i = n - 1;i > 0;i--) {
        std::swap(arr[0], arr[i]); s.swaps++;
        std::vector<int> snap = arr; int ss = i;
        s.steps.push_back([&s, snap, ss, n]() {
            s.bars = snap; resetColors(s); s.colorMap[0] = 2;
            for (int k = ss;k < n;k++) s.colorMap[k] = 3;
            });
        heapify(arr, s.steps, s.colorMap, s.comparisons, s.swaps, i, 0);
        std::vector<int> sn2 = arr; int si2 = i;
        s.steps.push_back([&s, sn2, si2, n]() {
            s.bars = sn2; resetColors(s);
            for (int k = si2;k < n;k++) s.colorMap[k] = 3;
            });
    }
    s.steps.push_back([&s, n]() {
        std::sort(s.bars.begin(), s.bars.end());
        for (int k = 0;k < n;k++) s.colorMap[k] = 3;
        });
}

static void buildSteps(SortState& s) {
    s.steps.clear(); s.stepIdx = 0; s.comparisons = s.swaps = 0; resetColors(s);
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

// ── Drawing ──────────────────────────────────────────────
static void drawBars(const SortState& s) {
    int n = (int)s.bars.size();
    int bw = (SW - BAR_GAP * (n + 1)) / n;
    int areaH = SH - PANEL_H - BOT_PAD;
    for (int i = 0;i < n;i++) {
        float t = (float)s.bars[i] / n;
        int bh = (int)(t * areaH);
        int bx = BAR_GAP + i * (bw + BAR_GAP);
        int by = PANEL_H + (areaH - bh);
        Color c;
        switch (s.colorMap[i]) {
        case 1: c = C_COMPARE; break;
        case 2: c = C_SWAP;    break;
        case 3: c = C_SORTED;  break;
        default:c = C_BAR;     break;
        }
        DrawRectangle(bx, by, bw, bh, c);
    }
}

static void drawPanel(const SortState& s) {
    DrawRectangle(0, 0, SW, PANEL_H, C_PANEL);
    DrawLine(0, PANEL_H, SW, PANEL_H, { 40,44,70,255 });

    // Algorithm buttons [1]..[6]
    int btnW = 130, btnH = 30, gap = 8, startX = 16, btnY = 8;
    for (int i = 0;i < ALGO_COUNT;i++) {
        bool active = (s.algo == i);
        Color bg = active ? C_ACCENT : C_BTN;
        Color tc = active ? C_BG : C_TEXT;
        DrawRectangleRounded({ (float)(startX + i * (btnW + gap)),(float)btnY,(float)btnW,(float)btnH },
            0.25f, 6, bg);
        char lbl[40]; snprintf(lbl, sizeof(lbl), "[%d] %s", i + 1, ALGO_NAMES[i]);
        int tw = MeasureText(lbl, 12);
        DrawText(lbl, startX + i * (btnW + gap) + (btnW - tw) / 2, btnY + 9, 12, tc);
    }

    // Status line
    const char* status = s.finished ? "SORTED" : s.running ? "RUNNING" : "PAUSED";
    DrawText(TextFormat("%s  %s  |  Comparisons: %lld  |  Swaps: %lld  |  Speed: %d",
        ALGO_CMPLX[s.algo], status,
        s.comparisons, s.swaps, s.speed),
        16, 46, 15, C_TEXT);
    DrawText("[SPACE] Start/Pause   [R] Shuffle   [UP/DOWN] Speed   [1-6] Algo",
        SW - 500, 50, 13, C_SUB);
}

int main() {
    InitWindow(SW, SH, "Sorting Visualizer - Day 3: All Algorithms");
    SetTargetFPS(60);

    SortState s;
    s.bars.resize(BAR_COUNT);
    s.colorMap.resize(BAR_COUNT, 0);
    shuffle(s);

    while (!WindowShouldClose()) {
        for (int i = 0;i < ALGO_COUNT;i++)
            if (IsKeyPressed(KEY_ONE + i)) { s.algo = (Algorithm)i;shuffle(s); }
        if (IsKeyPressed(KEY_R)) shuffle(s);
        if (IsKeyPressed(KEY_SPACE)) {
            if (s.finished) shuffle(s);
            else {
                if (!s.running && s.steps.empty()) buildSteps(s);
                s.running = !s.running;
            }
        }
        if (IsKeyPressed(KEY_UP))   s.speed = std::min(10, s.speed + 1);
        if (IsKeyPressed(KEY_DOWN)) s.speed = std::max(1, s.speed - 1);

        if (s.running && !s.finished) {
            int spf = (int)std::round(std::pow(2.8f, (s.speed - 1) / 3.0f));
            for (int k = 0;k < spf && s.stepIdx < s.steps.size();k++)
                s.steps[s.stepIdx++]();
            if (s.stepIdx >= s.steps.size()) {
                s.running = false; s.finished = true;
                for (auto& c : s.colorMap) c = 3;
            }
        }

        BeginDrawing();
        ClearBackground(C_BG);
        drawBars(s);
        drawPanel(s);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}