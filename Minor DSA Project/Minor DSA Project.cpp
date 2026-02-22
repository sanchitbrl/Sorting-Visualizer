#include "raylib.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <functional>
#include <cmath>
#include <cstdio>

static const int SW = 1600;
static const int SH = 900;
static const int BAR_COUNT = 100;
static const int BAR_GAP = 2;
static const int PANEL_H = 80;
static const int BOT_PAD = 20;
static const int BAR_AREA_Y = PANEL_H;
static const int BAR_AREA_H = SH - PANEL_H - BOT_PAD;

// ── Colour palette ───────────────────────────────────────
static const Color C_BG = { 10, 12, 20, 255 };
static const Color C_PANEL = { 18, 20, 34, 255 };
static const Color C_TEXT = { 220,224,240,255 };
static const Color C_SUB = { 120,128,160,255 };
static const Color C_ACCENT = { 99,155,255,255 };
static const Color C_BTN = { 32, 36, 58,255 };
// bar gradient pairs
static const Color C_BAR_LO = { 55,100,200,255 };
static const Color C_BAR_HI = { 90,160,255,255 };
static const Color C_CMP_LO = { 200,150, 20,255 };
static const Color C_CMP_HI = { 255,210, 60,255 };
static const Color C_SWP_LO = { 200, 40, 40,255 };
static const Color C_SWP_HI = { 255, 90, 90,255 };
static const Color C_SRT_LO = { 30,160, 80,255 };
static const Color C_SRT_HI = { 80,230,130,255 };

enum Algorithm { BUBBLE = 0, SELECTION, INSERTION, MERGE, QUICK, HEAP, ALGO_COUNT };
static const char* ALGO_NAMES[] = {
    "Bubble Sort","Selection Sort","Insertion Sort",
    "Merge Sort","Quick Sort","Heap Sort"
};
static const char* ALGO_CMPLX[] = {
    "O(n²)","O(n²)","O(n²)","O(n log n)","O(n log n)","O(n log n)"
};

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

static void resetColors(SortState& s) { std::fill(s.colorMap.begin(), s.colorMap.end(), 0); }

static void shuffle(SortState& s) {
    static std::mt19937 rng(std::random_device{}());
    std::iota(s.bars.begin(), s.bars.end(), 1);
    std::shuffle(s.bars.begin(), s.bars.end(), rng);
    resetColors(s);
    s.running = s.finished = false;
    s.steps.clear(); s.stepIdx = 0;
    s.comparisons = s.swaps = 0;
}

static Color lerpCol(Color a, Color b, float t) {
    return { (unsigned char)(a.r + (b.r - a.r) * t),
            (unsigned char)(a.g + (b.g - a.g) * t),
            (unsigned char)(a.b + (b.b - a.b) * t),255 };
}

// ── Sort builders (same as Day 3) ────────────────────────
static void buildBubble(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 0;i < n - 1;i++)
        for (int j = 0;j < n - 1 - i;j++)
            s.steps.push_back([&s, j, i, n]() {
            resetColors(s);
            s.colorMap[j] = 1; s.colorMap[j + 1] = 1; s.comparisons++;
            if (s.bars[j] > s.bars[j + 1]) { std::swap(s.bars[j], s.bars[j + 1]);s.colorMap[j] = 2;s.colorMap[j + 1] = 2;s.swaps++; }
            for (int k = n - i - 1;k < n;k++) s.colorMap[k] = 3;
                });
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}
static void buildSelection(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 0;i < n - 1;i++)
        s.steps.push_back([&s, i, n]() mutable {
        resetColors(s); int mi = i; s.colorMap[i] = 2;
        for (int j = i + 1;j < n;j++) { s.comparisons++;s.colorMap[j] = 1;if (s.bars[j] < s.bars[mi]) { if (mi != i)s.colorMap[mi] = 0;mi = j;s.colorMap[mi] = 2; } }
        if (mi != i) { std::swap(s.bars[i], s.bars[mi]);s.swaps++; }
        for (int k = 0;k <= i;k++) s.colorMap[k] = 3;
            });
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}
static void buildInsertion(SortState& s) {
    int n = (int)s.bars.size();
    for (int i = 1;i < n;i++)
        s.steps.push_back([&s, i]() {
        resetColors(s); int key = s.bars[i], j = i - 1; s.colorMap[i] = 2;
        while (j >= 0 && s.bars[j] > key) { s.bars[j + 1] = s.bars[j];s.colorMap[j + 1] = 1;j--;s.comparisons++;s.swaps++; }
        s.bars[j + 1] = key; s.colorMap[j + 1] = 2;
            });
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}
static void buildMerge(SortState& s) {
    int n = (int)s.bars.size();
    for (int w = 1;w < n;w *= 2)
        for (int i = 0;i < n;i += 2 * w) {
            int l = i, m = std::min(i + w - 1, n - 1), r = std::min(i + 2 * w - 1, n - 1);
            if (m >= r) continue;
            s.steps.push_back([&s, l, m, r]() {
                resetColors(s);
                std::vector<int> tmp(s.bars.begin() + l, s.bars.begin() + r + 1);
                int i2 = 0, j2 = m - l + 1, k = l;
                while (i2 <= m - l && j2 <= r - l) { s.comparisons++;if (tmp[i2] <= tmp[j2])s.bars[k++] = tmp[i2++];else { s.bars[k++] = tmp[j2++];s.swaps++; } }
                while (i2 <= m - l) s.bars[k++] = tmp[i2++];
                while (j2 <= r - l) s.bars[k++] = tmp[j2++];
                for (int x = l;x <= r;x++) s.colorMap[x] = 2;
                });
        }
    s.steps.push_back([&s, n]() { for (int k = 0;k < n;k++) s.colorMap[k] = 3; });
}
static void buildQuick(SortState& s) {
    int n = (int)s.bars.size(); struct Range { int l, r; };
    std::vector<int> arr = s.bars; std::vector<Range> work; work.push_back({ 0,n - 1 });
    while (!work.empty()) {
        Range wr = work.back(); work.pop_back(); int l = wr.l, r = wr.r;
        if (l >= r) continue;
        int pivot = arr[r], i2 = l - 1;
        std::vector<int> snap = arr; int sL = l, sR = r;
        s.steps.push_back([&s, snap, sL, sR]() mutable {
            resetColors(s); int p2 = s.bars[sR], i3 = sL - 1; s.colorMap[sR] = 2;
            for (int j = sL;j < sR;j++) { s.comparisons++;s.colorMap[j] = 1;if (s.bars[j] <= p2) { i3++;std::swap(s.bars[i3], s.bars[j]);s.colorMap[i3] = 2;s.swaps++; } }
            std::swap(s.bars[i3 + 1], s.bars[sR]); s.colorMap[i3 + 1] = 3;
            });
        for (int j = l;j < r;j++) if (arr[j] <= pivot) { i2++;std::swap(arr[i2], arr[j]); }
        std::swap(arr[i2 + 1], arr[r]); int p = i2 + 1;
        if (p - 1 > l) work.push_back({ l,p - 1 });
        if (p + 1 < r) work.push_back({ p + 1,r });
    }
    s.steps.push_back([&s, n]() {std::sort(s.bars.begin(), s.bars.end());for (int k = 0;k < n;k++) s.colorMap[k] = 3;});
}
static void heapify(std::vector<int>& arr, std::vector<std::function<void()>>& steps,
    std::vector<int>& cm, long long& cc, long long& sc, int n, int i) {
    int lg = i, l = 2 * i + 1, r = 2 * i + 2;
    if (l < n) { cc++;if (arr[l] > arr[lg])lg = l; } if (r < n) { cc++;if (arr[r] > arr[lg])lg = r; }
    if (lg != i) {
        std::swap(arr[i], arr[lg]);sc++;int ci = i, cl = lg;
        steps.push_back([&cm, ci, cl]() {std::fill(cm.begin(), cm.end(), 0);cm[ci] = 2;cm[cl] = 1;});
        heapify(arr, steps, cm, cc, sc, n, lg);
    }
}
static void buildHeap(SortState& s) {
    int n = (int)s.bars.size(); std::vector<int> arr = s.bars;
    for (int i = n / 2 - 1;i >= 0;i--) { heapify(arr, s.steps, s.colorMap, s.comparisons, s.swaps, n, i);std::vector<int> snap = arr;s.steps.push_back([&s, snap]() {s.bars = snap;resetColors(s);}); }
    for (int i = n - 1;i > 0;i--) {
        std::swap(arr[0], arr[i]);s.swaps++;std::vector<int> snap = arr;int ss = i;
        s.steps.push_back([&s, snap, ss, n]() {s.bars = snap;resetColors(s);s.colorMap[0] = 2;for (int k = ss;k < n;k++)s.colorMap[k] = 3;});
        heapify(arr, s.steps, s.colorMap, s.comparisons, s.swaps, i, 0);std::vector<int> sn2 = arr;int si2 = i;
        s.steps.push_back([&s, sn2, si2, n]() {s.bars = sn2;resetColors(s);for (int k = si2;k < n;k++)s.colorMap[k] = 3;});
    }
    s.steps.push_back([&s, n]() {std::sort(s.bars.begin(), s.bars.end());for (int k = 0;k < n;k++)s.colorMap[k] = 3;});
}
static void buildSteps(SortState& s) {
    s.steps.clear();s.stepIdx = 0;s.comparisons = s.swaps = 0;resetColors(s);
    switch (s.algo) {
    case BUBBLE:buildBubble(s);break; case SELECTION:buildSelection(s);break;
    case INSERTION:buildInsertion(s);break; case MERGE:buildMerge(s);break;
    case QUICK:buildQuick(s);break; case HEAP:buildHeap(s);break; default:break;
    }
}

// ══ NEW: Polished drawing helpers ════════════════════════

// Vertical gradient bar with a bright rounded cap on top
static void drawGradBar(int x, int y, int w, int h, Color lo, Color hi) {
    if (h <= 0) return;
    DrawRectangleGradientV(x, y, w, h, hi, lo);
    int capH = std::min(h, 5);
    DrawRectangleRounded({ (float)x,(float)y,(float)w,(float)capH }, 0.5f, 4, hi);
}

// Soft ambient glow behind a bar
static void drawGlow(int x, int y, int w, int h, Color c) {
    DrawRectangle(x - 2, y - 3, w + 4, h + 3, { c.r,c.g,c.b,35 });
}

static void drawBars(const SortState& s) {
    int n = (int)s.bars.size();
    int bw = (SW - BAR_GAP * (n + 1)) / n;

    // ── Grid lines (NEW) ─────────────────────────────────
    for (int pct = 25;pct < 100;pct += 25) {
        int gy = BAR_AREA_Y + (int)(BAR_AREA_H * (1.f - pct / 100.f));
        DrawLine(0, gy, SW, gy, { 30,36,60,255 });
        DrawText(TextFormat("%d%%", pct), 5, gy - 13, 11, { 46,54,86,255 });
    }

    for (int i = 0;i < n;i++) {
        float t = (float)s.bars[i] / n;
        int bh = (int)(t * BAR_AREA_H);
        int bx = BAR_GAP + i * (bw + BAR_GAP);
        int by = BAR_AREA_Y + (BAR_AREA_H - bh);

        Color lo, hi;
        switch (s.colorMap[i]) {
        case 1: lo = C_CMP_LO;hi = C_CMP_HI; drawGlow(bx, by, bw, bh, hi); break;
        case 2: lo = C_SWP_LO;hi = C_SWP_HI; drawGlow(bx, by, bw, bh, hi); break;
        case 3: lo = C_SRT_LO;hi = C_SRT_HI; break;
        default:lo = C_BAR_LO;hi = C_BAR_HI; break;
        }
        drawGradBar(bx, by, bw, bh, lo, hi);   // ← gradient instead of flat rect
    }
}

static void drawPanel(const SortState& s) {
    DrawRectangle(0, 0, SW, PANEL_H, C_PANEL);
    DrawLine(0, PANEL_H, SW, PANEL_H, { 40,44,70,255 });

    // Algo buttons
    int btnW = 130, btnH = 30, gap = 8, startX = 16, btnY = 8;
    for (int i = 0;i < ALGO_COUNT;i++) {
        bool active = (s.algo == i);
        Color bg = active ? C_ACCENT : C_BTN; Color tc = active ? C_BG : C_TEXT;
        DrawRectangleRounded({ (float)(startX + i * (btnW + gap)),(float)btnY,(float)btnW,(float)btnH }, 0.25f, 6, bg);
        char lbl[40]; snprintf(lbl, sizeof(lbl), "[%d] %s", i + 1, ALGO_NAMES[i]);
        int tw = MeasureText(lbl, 12);
        DrawText(lbl, startX + i * (btnW + gap) + (btnW - tw) / 2, btnY + 9, 12, tc);
    }

    // Status + stats
    const char* status = s.finished ? "SORTED" : s.running ? "RUNNING" : "PAUSED";
    DrawText(TextFormat("%s  %s  |  Comparisons: %lld  |  Swaps: %lld",
        ALGO_CMPLX[s.algo], status, s.comparisons, s.swaps),
        16, 46, 15, C_TEXT);

    // ── Speed bar (colour gradient – NEW) ────────────────
    int spX = SW - 220, spY = 46;
    DrawText("Speed", spX, spY, 13, C_SUB);
    int sBX = spX + 55, sBY = spY, sBW = 130, sBH = 13;
    DrawRectangleRounded({ (float)sBX,(float)sBY,(float)sBW,(float)sBH }, 0.5f, 6, { 32,36,58,255 });
    float sf = (s.speed - 1) / 9.f;
    Color sc = lerpCol({ 60,200,100,255 }, { 255,90,50,255 }, sf);
    if (sf > 0.f) DrawRectangleRounded({ (float)sBX,(float)sBY,(float)(sBW * sf),(float)sBH }, 0.5f, 6, sc);
    DrawText(TextFormat("%d", s.speed), sBX + sBW + 6, spY, 16, C_TEXT);

    DrawText("[SPACE] Start/Pause  [R] Shuffle  [UP/DOWN] Speed  [1-6] Algo",
        16, 64, 12, C_SUB);
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "Sorting Visualizer - Day 4: Visual Polish");
    SetTargetFPS(60);

    SortState s;
    s.bars.resize(BAR_COUNT); s.colorMap.resize(BAR_COUNT, 0);
    shuffle(s);

    while (!WindowShouldClose()) {
        for (int i = 0;i < ALGO_COUNT;i++)
            if (IsKeyPressed(KEY_ONE + i)) { s.algo = (Algorithm)i;shuffle(s); }
        if (IsKeyPressed(KEY_R)) shuffle(s);
        if (IsKeyPressed(KEY_SPACE)) {
            if (s.finished) shuffle(s);
            else { if (!s.running && s.steps.empty()) buildSteps(s); s.running = !s.running; }
        }
        if (IsKeyPressed(KEY_UP))   s.speed = std::min(10, s.speed + 1);
        if (IsKeyPressed(KEY_DOWN)) s.speed = std::max(1, s.speed - 1);

        if (s.running && !s.finished) {
            int spf = (int)std::round(std::pow(2.8f, (s.speed - 1) / 3.0f));
            for (int k = 0;k < spf && s.stepIdx < s.steps.size();k++) s.steps[s.stepIdx++]();
            if (s.stepIdx >= s.steps.size()) { s.running = false;s.finished = true;for (auto& c : s.colorMap)c = 3; }
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