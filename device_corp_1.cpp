// «Девайс-Корп» — симулятор компании по разработке электроники.
// Один файл, C++14 + SDL2. Без внешних ассетов: графика — примитивы, шрифт — зашитый массив глифов,
// звук — процедурный синтез.
// Сборка: g++ -std=c++14 -O2 -Wall -Wextra device_corp.cpp -o device_corp $(sdl2-config --cflags --libs)

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

// ============================================================================
//  Базовое
// ============================================================================
const int W = 1280, H = 720;
SDL_Renderer* R = nullptr;
std::mt19937 rng(12345u);

double frand() { return std::uniform_real_distribution<double>(0.0, 1.0)(rng); }
int irand(int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); }
double clampd(double v, double a, double b) { return v < a ? a : (v > b ? b : v); }
int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }

struct Col { Uint8 r, g, b, a; };
const Col C_BG     = {16, 20, 32, 255};
const Col C_PANEL  = {28, 34, 52, 255};
const Col C_PANEL2 = {22, 27, 42, 255};
const Col C_BORDER = {58, 70, 104, 255};
const Col C_TEXT   = {232, 236, 246, 255};
const Col C_DIM    = {140, 152, 178, 255};
const Col C_ACC    = {70, 180, 255, 255};
const Col C_GREEN  = {80, 210, 120, 255};
const Col C_RED    = {235, 85, 85, 255};
const Col C_YEL    = {255, 214, 40, 255};
const Col C_ORANGE = {255, 150, 60, 255};
const Col C_BTN    = {48, 60, 92, 255};
const Col C_BTNH   = {66, 82, 124, 255};
const Col C_BTNSEL = {36, 120, 200, 255};
const Col C_BTND   = {36, 40, 54, 255};

Col shade(Col c, double k) {
    Col o;
    o.r = (Uint8)clampi((int)(c.r * k), 0, 255);
    o.g = (Uint8)clampi((int)(c.g * k), 0, 255);
    o.b = (Uint8)clampi((int)(c.b * k), 0, 255);
    o.a = c.a;
    return o;
}

void setc(Col c) { SDL_SetRenderDrawColor(R, c.r, c.g, c.b, c.a); }
void fillRect(int x, int y, int w, int h, Col c) {
    if (w <= 0 || h <= 0) return;
    setc(c);
    SDL_Rect rr = {x, y, w, h};
    SDL_RenderFillRect(R, &rr);
}
void fillRect(const SDL_Rect& r, Col c) { fillRect(r.x, r.y, r.w, r.h, c); }
void drawRect(const SDL_Rect& r, Col c, int t = 1) {
    fillRect(r.x, r.y, r.w, t, c);
    fillRect(r.x, r.y + r.h - t, r.w, t, c);
    fillRect(r.x, r.y, t, r.h, c);
    fillRect(r.x + r.w - t, r.y, t, r.h, c);
}
void fillCircle(int cx, int cy, int rad, Col c) {
    setc(c);
    for (int dy = -rad; dy <= rad; ++dy) {
        int dx = (int)std::sqrt((double)(rad * rad - dy * dy));
        SDL_RenderDrawLine(R, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}
void fillRound(SDL_Rect r, int rad, Col c) {
    rad = std::min(rad, std::min(r.w, r.h) / 2);
    if (rad <= 0) { fillRect(r, c); return; }
    fillRect(r.x + rad, r.y, r.w - 2 * rad, r.h, c);
    fillRect(r.x, r.y + rad, rad, r.h - 2 * rad, c);
    fillRect(r.x + r.w - rad, r.y + rad, rad, r.h - 2 * rad, c);
    fillCircle(r.x + rad, r.y + rad, rad, c);
    fillCircle(r.x + r.w - rad - 1, r.y + rad, rad, c);
    fillCircle(r.x + rad, r.y + r.h - rad - 1, rad, c);
    fillCircle(r.x + r.w - rad - 1, r.y + r.h - rad - 1, rad, c);
}
bool inRect(int x, int y, const SDL_Rect& r) { return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h; }

// ============================================================================
//  Растровый шрифт 5x7 с кириллицей (строчные рисуются капителью)
// ============================================================================
struct Glyph { unsigned cp; unsigned char r[7]; };
const Glyph FONT[] = {
    {' ', {0, 0, 0, 0, 0, 0, 0}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'"', {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'#', {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A}},
    {'$', {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}},
    {'%', {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}},
    {'\'', {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
    {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
    {'*', {0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'/', {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
    {';', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08}},
    {'<', {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}},
    {'=', {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00}},
    {'>', {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'[', {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}},
    {']', {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}},
    {'_', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}},
    {'|', {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {0xAB, {0x00, 0x05, 0x0A, 0x14, 0x0A, 0x05, 0x00}},     // «
    {0xBB, {0x00, 0x14, 0x0A, 0x05, 0x0A, 0x14, 0x00}},     // »
    {0x2014, {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},   // —
    {0x2248, {0x00, 0x08, 0x15, 0x02, 0x08, 0x15, 0x02}},   // ≈
    {0x2265, {0x08, 0x04, 0x02, 0x04, 0x08, 0x00, 0x1F}},   // ≥
    // Кириллица (прописные)
    {0x0401, {0x0A, 0x1F, 0x10, 0x1E, 0x10, 0x10, 0x1F}},   // Ё
    {0x0410, {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},   // А
    {0x0411, {0x1F, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x1E}},   // Б
    {0x0412, {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},   // В
    {0x0413, {0x1F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}},   // Г
    {0x0414, {0x06, 0x0A, 0x0A, 0x0A, 0x0A, 0x1F, 0x11}},   // Д
    {0x0415, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},   // Е
    {0x0416, {0x15, 0x15, 0x15, 0x0E, 0x15, 0x15, 0x15}},   // Ж
    {0x0417, {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}},   // З
    {0x0418, {0x11, 0x11, 0x13, 0x15, 0x19, 0x11, 0x11}},   // И
    {0x0419, {0x0A, 0x04, 0x11, 0x13, 0x15, 0x19, 0x11}},   // Й
    {0x041A, {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},   // К
    {0x041B, {0x07, 0x09, 0x09, 0x09, 0x09, 0x09, 0x11}},   // Л
    {0x041C, {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},   // М
    {0x041D, {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},   // Н
    {0x041E, {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},   // О
    {0x041F, {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}},   // П
    {0x0420, {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},   // Р
    {0x0421, {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},   // С
    {0x0422, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},   // Т
    {0x0423, {0x11, 0x11, 0x11, 0x0F, 0x01, 0x11, 0x0E}},   // У
    {0x0424, {0x04, 0x0E, 0x15, 0x15, 0x15, 0x0E, 0x04}},   // Ф
    {0x0425, {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},   // Х
    {0x0426, {0x12, 0x12, 0x12, 0x12, 0x12, 0x1F, 0x01}},   // Ц
    {0x0427, {0x11, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x01}},   // Ч
    {0x0428, {0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x1F}},   // Ш
    {0x0429, {0x15, 0x15, 0x15, 0x15, 0x15, 0x1F, 0x01}},   // Щ
    {0x042A, {0x18, 0x08, 0x08, 0x0E, 0x09, 0x09, 0x0E}},   // Ъ
    {0x042B, {0x11, 0x11, 0x11, 0x1D, 0x13, 0x13, 0x1D}},   // Ы
    {0x042C, {0x10, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x1E}},   // Ь
    {0x042D, {0x0E, 0x11, 0x01, 0x07, 0x01, 0x11, 0x0E}},   // Э
    {0x042E, {0x12, 0x15, 0x15, 0x1D, 0x15, 0x15, 0x12}},   // Ю
    {0x042F, {0x0F, 0x11, 0x11, 0x0F, 0x05, 0x09, 0x11}},   // Я
};
const int FONT_N = (int)(sizeof(FONT) / sizeof(FONT[0]));
int g_glyphIdx[0x460];

void initFont() {
    for (int i = 0; i < 0x460; ++i) g_glyphIdx[i] = -1;
    for (int k = 0; k < FONT_N; ++k)
        if (FONT[k].cp < 0x460) g_glyphIdx[FONT[k].cp] = k;
}

unsigned nextCp(const std::string& s, size_t& i) {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { i += 1; return c; }
    if ((c >> 5) == 6 && i + 1 < s.size()) {
        unsigned cp = ((unsigned)(c & 0x1F) << 6) | ((unsigned char)s[i + 1] & 0x3Fu);
        i += 2; return cp;
    }
    if ((c >> 4) == 14 && i + 2 < s.size()) {
        unsigned cp = ((unsigned)(c & 0x0F) << 12) | (((unsigned char)s[i + 1] & 0x3Fu) << 6) |
                      ((unsigned char)s[i + 2] & 0x3Fu);
        i += 3; return cp;
    }
    if ((c >> 3) == 30) { i += 4; return '?'; }
    i += 1;
    return '?';
}

const unsigned char* glyphFor(unsigned cp, bool& small) {
    small = false;
    if (cp >= 'a' && cp <= 'z') { cp -= 32; small = true; }
    else if (cp >= 0x430 && cp <= 0x44F) { cp -= 0x20; small = true; }
    else if (cp == 0x451) { cp = 0x401; small = true; }
    if (cp < 0x460) {
        int k = g_glyphIdx[cp];
        if (k >= 0) return FONT[k].r;
    } else {
        for (int k = 0; k < FONT_N; ++k) if (FONT[k].cp == cp) return FONT[k].r;
    }
    return FONT[g_glyphIdx[(unsigned)'?']].r;
}

int cpLen(const std::string& s) {
    int n = 0; size_t i = 0;
    while (i < s.size()) { nextCp(s, i); ++n; }
    return n;
}
int textW(const std::string& s, int sc) { int n = cpLen(s); return n > 0 ? n * 6 * sc - sc : 0; }

std::vector<SDL_Rect> g_rects;
void drawText(const std::string& s, int x, int y, int sc, Col c) {
    g_rects.clear();
    size_t i = 0;
    int cx = x;
    while (i < s.size()) {
        unsigned cp = nextCp(s, i);
        bool small;
        const unsigned char* g = glyphFor(cp, small);
        for (int r = 0; r < 7; ++r) {
            int y0, y1;
            if (small) {
                y0 = y + 2 * sc + (r * 5 * sc) / 7;
                y1 = y + 2 * sc + ((r + 1) * 5 * sc) / 7;
                if (y1 <= y0) y1 = y0 + 1;
            } else {
                y0 = y + r * sc; y1 = y0 + sc;
            }
            for (int col = 0; col < 5; ++col)
                if (g[r] & (0x10 >> col)) {
                    SDL_Rect rr = {cx + col * sc, y0, sc, y1 - y0};
                    g_rects.push_back(rr);
                }
        }
        cx += 6 * sc;
    }
    if (!g_rects.empty()) {
        setc(c);
        SDL_RenderFillRects(R, g_rects.data(), (int)g_rects.size());
    }
}
std::string fitText(const std::string& s, int sc, int maxw) {
    if (textW(s, sc) <= maxw) return s;
    std::vector<size_t> starts;
    size_t i = 0;
    while (i < s.size()) { starts.push_back(i); nextCp(s, i); }
    for (int n = (int)starts.size() - 1; n > 0; --n) {
        std::string t = s.substr(0, starts[(size_t)n]) + "..";
        if (textW(t, sc) <= maxw) return t;
    }
    return "..";
}
void drawTextC(const std::string& s, int cx, int y, int sc, Col c) { drawText(s, cx - textW(s, sc) / 2, y, sc, c); }
void drawTextR(const std::string& s, int rx, int y, int sc, Col c) { drawText(s, rx - textW(s, sc), y, sc, c); }

std::string fmtNum(double v) {
    long long n = (long long)std::llround(std::fabs(v));
    std::string s = std::to_string(n), o;
    int c = 0;
    for (int i = (int)s.size() - 1; i >= 0; --i) {
        o.insert(o.begin(), s[(size_t)i]);
        if (++c % 3 == 0 && i > 0) o.insert(o.begin(), ' ');
    }
    return (v < -0.5 ? std::string("-") : std::string()) + o;
}
std::string fmtMoney(double v) {
    std::string n = fmtNum(std::fabs(v));
    return (v < -0.5 ? "-$" : "$") + n;
}
std::string fmtShort(double v) {
    char b[32];
    double a = std::fabs(v);
    if (a >= 1e6) std::snprintf(b, sizeof b, "%.1fМ", v / 1e6);
    else if (a >= 1e3) std::snprintf(b, sizeof b, "%.0fк", v / 1e3);
    else std::snprintf(b, sizeof b, "%.0f", v);
    return b;
}
std::string fmt1(double v) { char b[32]; std::snprintf(b, sizeof b, "%.1f", v); return b; }

// ============================================================================
//  Процедурный звук
// ============================================================================
enum { SFX_CLICK, SFX_BACK, SFX_ERR, SFX_NAV, SFX_CASH, SFX_DONE, SFX_LAUNCH, SFX_BAD };
struct Voice { float f0, f1, dur, t, delay, vol, ph; int wave; };
struct AudioSys {
    SDL_AudioDeviceID dev = 0;
    int rate = 44100;
    std::vector<Voice> v;
    float vol = 0.6f;
    Uint32 seed = 22222u;
};
AudioSys A;

void SDLCALL audioCb(void* ud, Uint8* stream, int len) {
    AudioSys* a = static_cast<AudioSys*>(ud);
    Sint16* out = reinterpret_cast<Sint16*>(stream);
    int n = len / 2;
    float dt = 1.0f / (float)a->rate;
    for (int i = 0; i < n; ++i) {
        float s = 0.0f;
        for (size_t k = 0; k < a->v.size(); ++k) {
            Voice& vc = a->v[k];
            if (vc.delay > 0.0f) { vc.delay -= dt; continue; }
            if (vc.t >= vc.dur) continue;
            float q = vc.t / vc.dur;
            float f = vc.f0 + (vc.f1 - vc.f0) * q;
            vc.ph += f * dt;
            vc.ph -= std::floor(vc.ph);
            float w;
            switch (vc.wave) {
                case 0: w = std::sin(vc.ph * 6.2831853f); break;
                case 1: w = vc.ph < 0.5f ? 0.6f : -0.6f; break;
                case 2:
                    a->seed = a->seed * 1664525u + 1013904223u;
                    w = (float)((a->seed >> 9) & 0xFFFFu) / 32768.0f - 1.0f;
                    break;
                default: w = 4.0f * std::fabs(vc.ph - 0.5f) - 1.0f; break;
            }
            float env = std::min(1.0f, vc.t * 200.0f) * (1.0f - q);
            s += w * env * vc.vol;
            vc.t += dt;
        }
        s *= a->vol;
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        out[i] = (Sint16)(s * 30000.0f);
    }
    a->v.erase(std::remove_if(a->v.begin(), a->v.end(),
                              [](const Voice& vc) { return vc.delay <= 0.0f && vc.t >= vc.dur; }),
               a->v.end());
}

void addVoice(float f0, float f1, float dur, float delay, float vol, int wave) {
    Voice v;
    v.f0 = f0; v.f1 = f1; v.dur = dur; v.t = 0.0f; v.delay = delay; v.vol = vol; v.ph = 0.0f; v.wave = wave;
    A.v.push_back(v);
}

void sfx(int id) {
    if (!A.dev) return;
    SDL_LockAudioDevice(A.dev);
    if (A.v.size() < 48) {
        switch (id) {
            case SFX_CLICK: addVoice(660, 760, 0.05f, 0, 0.30f, 0); break;
            case SFX_BACK: addVoice(520, 360, 0.07f, 0, 0.28f, 0); break;
            case SFX_ERR: addVoice(190, 150, 0.16f, 0, 0.18f, 1); break;
            case SFX_NAV: addVoice(980, 980, 0.025f, 0, 0.12f, 0); break;
            case SFX_CASH:
                addVoice(1250, 1250, 0.06f, 0, 0.22f, 0);
                addVoice(1680, 1680, 0.10f, 0.06f, 0.22f, 0);
                break;
            case SFX_DONE:
                addVoice(523, 523, 0.12f, 0.00f, 0.25f, 3);
                addVoice(659, 659, 0.12f, 0.10f, 0.25f, 3);
                addVoice(784, 784, 0.22f, 0.20f, 0.25f, 3);
                break;
            case SFX_LAUNCH:
                addVoice(392, 392, 0.14f, 0.00f, 0.25f, 3);
                addVoice(523, 523, 0.14f, 0.12f, 0.25f, 3);
                addVoice(659, 659, 0.14f, 0.24f, 0.25f, 3);
                addVoice(1046, 1046, 0.45f, 0.36f, 0.25f, 3);
                addVoice(0, 0, 0.6f, 0.36f, 0.10f, 2);
                break;
            case SFX_BAD:
                addVoice(330, 300, 0.2f, 0.0f, 0.2f, 3);
                addVoice(262, 220, 0.4f, 0.18f, 0.2f, 3);
                break;
            default: break;
        }
    }
    SDL_UnlockAudioDevice(A.dev);
}

// ============================================================================
//  Немедленный UI: кнопки, фокус для геймпада, прокрутка, ползунки
// ============================================================================
struct WInfo { SDL_Rect r; int area; };
struct AInfo { int id; SDL_Rect v; };
struct UIState {
    std::vector<WInfo> pw, cw;
    std::vector<AInfo> pa, ca;
    std::vector<SDL_Rect> psl, csl;
    int focus = 0;
    bool pad = false;
    int activate = -1;
    bool focusMoved = false;
    bool click = false;
    int cx = 0, cy = 0;
    bool mdown = false;
    int mx = -1000, my = -1000, dx0 = 0, dy0 = 0, lastY = 0;
    bool dragging = false;
    int dragArea = -1;
    bool sliderPress = false;
    int area = -1;
    SDL_Rect view{0, 0, 0, 0};
    float scroll[16] = {};
    float maxScroll[16] = {};
    int lastIdx = -1;
    bool reset = false;
};
UIState U;

bool widget(const SDL_Rect& r, bool enabled) {
    int idx = (int)U.cw.size();
    WInfo wi; wi.r = r; wi.area = U.area;
    U.cw.push_back(wi);
    U.lastIdx = idx;
    bool hit = false;
    if (U.click && inRect(U.cx, U.cy, r) && (U.area < 0 || inRect(U.cx, U.cy, U.view))) {
        U.click = false;
        if (enabled) hit = true; else sfx(SFX_ERR);
    }
    if (U.activate == idx) {
        U.activate = -1;
        if (enabled) hit = true; else sfx(SFX_ERR);
    }
    return hit;
}
bool isFocused() { return U.pad && U.lastIdx == U.focus; }
void drawFocus(const SDL_Rect& r) {
    if (!isFocused()) return;
    SDL_Rect f = {r.x - 3, r.y - 3, r.w + 6, r.h + 6};
    drawRect(f, C_YEL, 3);
}
bool hovered(const SDL_Rect& r) {
    return !U.pad && inRect(U.mx, U.my, r) && (U.area < 0 || inRect(U.mx, U.my, U.view));
}
bool button(const SDL_Rect& r, const std::string& label, bool enabled = true, bool sel = false, int sc = 2) {
    bool hit = widget(r, enabled);
    bool hov = hovered(r);
    Col c = !enabled ? C_BTND : (sel ? C_BTNSEL : (hov ? C_BTNH : C_BTN));
    if (enabled && hov && U.mdown && !U.dragging) c = shade(C_BTNSEL, 0.8);
    fillRect(r, c);
    fillRect(r.x, r.y, r.w, 2, shade(c, 1.35));
    drawRect(r, shade(c, 0.7));
    std::string t = fitText(label, sc, r.w - 12);
    drawTextC(t, r.x + r.w / 2, r.y + (r.h - 7 * sc) / 2, sc, enabled ? C_TEXT : C_DIM);
    drawFocus(r);
    if (hit) sfx(SFX_CLICK);
    return hit;
}

void beginArea(int id, SDL_Rect v) {
    U.area = id;
    U.view = v;
    AInfo ai; ai.id = id; ai.v = v;
    U.ca.push_back(ai);
    SDL_RenderSetClipRect(R, &v);
}
int scr(int id) { return (int)U.scroll[id]; }
void endArea(int contentH) {
    int id = U.area;
    SDL_RenderSetClipRect(R, nullptr);
    float mx = (float)std::max(0, contentH - U.view.h);
    U.maxScroll[id] = mx;
    if (U.scroll[id] > mx) U.scroll[id] = mx;
    if (U.scroll[id] < 0) U.scroll[id] = 0;
    if (mx > 0) {
        int th = std::max(30, U.view.h * U.view.h / std::max(1, contentH));
        int ty = U.view.y + (int)((U.view.h - th) * (U.scroll[id] / mx));
        fillRect(U.view.x + U.view.w - 5, U.view.y, 4, U.view.h, C_PANEL2);
        fillRect(U.view.x + U.view.w - 5, ty, 4, th, C_DIM);
    }
    U.area = -1;
}

// Ползунок с дискретными значениями: тап и перетаскивание
bool slider(const SDL_Rect& r, int count, int& val, const bool* allowed) {
    U.csl.push_back(r);
    if (U.mdown && U.sliderPress && inRect(U.dx0, U.dy0, r) && (U.area < 0 || inRect(U.dx0, U.dy0, U.view))) {
        int seg = clampi((U.mx - r.x) * count / std::max(1, r.w), 0, count - 1);
        if (allowed[seg] && seg != val) { val = seg; sfx(SFX_NAV); return true; }
    }
    return false;
}

void uiNav(int dir) {  // 0 вверх 1 вниз 2 влево 3 вправо
    if (!U.pad) { U.pad = true; return; }
    int n = (int)U.pw.size();
    if (n == 0) return;
    if (U.focus >= n) U.focus = 0;
    const SDL_Rect& a = U.pw[(size_t)U.focus].r;
    int ax = a.x + a.w / 2, ay = a.y + a.h / 2;
    int best = -1;
    double bestScore = 1e18;
    for (int i = 0; i < n; ++i) {
        if (i == U.focus) continue;
        const SDL_Rect& b = U.pw[(size_t)i].r;
        int bx = b.x + b.w / 2, by = b.y + b.h / 2;
        int dx = bx - ax, dy = by - ay;
        double prim, sec;
        if (dir == 0) { if (dy >= -4) continue; prim = -dy; sec = std::abs(dx); }
        else if (dir == 1) { if (dy <= 4) continue; prim = dy; sec = std::abs(dx); }
        else if (dir == 2) { if (dx >= -4) continue; prim = -dx; sec = std::abs(dy); }
        else { if (dx <= 4) continue; prim = dx; sec = std::abs(dy); }
        double score = prim + sec * 2.5;
        if (score < bestScore) { bestScore = score; best = i; }
    }
    if (best >= 0) { U.focus = best; U.focusMoved = true; sfx(SFX_NAV); }
}
void uiActivate() {
    if (!U.pad) { U.pad = true; return; }
    U.activate = U.focus;
}
int areaAt(int x, int y) {
    for (size_t i = 0; i < U.pa.size(); ++i) if (inRect(x, y, U.pa[i].v)) return U.pa[i].id;
    return -1;
}
void scrollBy(int id, float d) {
    if (id < 0 || id >= 16) return;
    U.scroll[id] = (float)clampd(U.scroll[id] + d, 0.0, U.maxScroll[id]);
}
void uiStick(float v, float dt) {
    if (std::fabs(v) < 0.25f) return;
    int area = -1;
    if (U.focus < (int)U.pw.size() && U.pw[(size_t)U.focus].area >= 0) area = U.pw[(size_t)U.focus].area;
    else if (!U.pa.empty()) area = U.pa[0].id;
    if (area < 0) return;
    U.pad = true;
    scrollBy(area, v * dt * 900.0f);
}
void uiEndFrame() {
    if (U.reset) {
        U.cw.clear(); U.ca.clear(); U.csl.clear();
        U.focus = 0;
        for (int i = 0; i < 16; ++i) { U.scroll[i] = 0; U.maxScroll[i] = 0; }
        U.reset = false;
    } else if (U.pad && U.focus < (int)U.cw.size()) {
        const WInfo& w = U.cw[(size_t)U.focus];
        if (w.area >= 0) {
            SDL_Rect v{0, 0, 0, 0};
            for (size_t i = 0; i < U.ca.size(); ++i) if (U.ca[i].id == w.area) v = U.ca[i].v;
            if (v.h > 0) {
                if (U.focusMoved) {
                    if (w.r.y < v.y) scrollBy(w.area, (float)(w.r.y - v.y - 8));
                    else if (w.r.y + w.r.h > v.y + v.h) scrollBy(w.area, (float)(w.r.y + w.r.h - v.y - v.h + 8));
                } else if (w.r.y + w.r.h <= v.y || w.r.y >= v.y + v.h) {
                    // прокрутили стиком: переводим фокус на видимый элемент
                    for (size_t i = 0; i < U.cw.size(); ++i)
                        if (U.cw[i].area == w.area && U.cw[i].r.y >= v.y && U.cw[i].r.y + U.cw[i].r.h <= v.y + v.h) {
                            U.focus = (int)i; break;
                        }
                }
            }
        }
    }
    if (U.focus >= (int)U.cw.size()) U.focus = U.cw.empty() ? 0 : (int)U.cw.size() - 1;
    U.focusMoved = false;
    U.pw.swap(U.cw); U.cw.clear();
    U.pa.swap(U.ca); U.ca.clear();
    U.psl.swap(U.csl); U.csl.clear();
    U.click = false;
    U.activate = -1;
}

void panel(int x, int y, int w, int h, const std::string& title) {
    SDL_Rect r = {x, y, w, h};
    fillRect(r, C_PANEL);
    drawRect(r, C_BORDER);
    if (!title.empty()) drawText(title, x + 12, y + 10, 2, C_ACC);
}
void progressBar(SDL_Rect r, double f, Col c) {
    fillRect(r, C_PANEL2);
    fillRect(r.x, r.y, (int)(r.w * clampd(f, 0, 1)), r.h, c);
    drawRect(r, C_BORDER);
}

// ============================================================================
//  Игровые данные
// ============================================================================
enum { P_SIZE, P_COLOR, P_SCREEN, P_CPU, P_BATT, P_SPK, P_PACK, P_CAMN, P_CAMRES, P_CAMSZ, P_OS, P_CPUSRC, NPARAM };
const char* PARAM_NAMES[NPARAM] = {"Размер", "Цвет", "Экран", "Процессор", "Батарея", "Динамики", "Упаковка",
                                    "Камеры", "Разрешение", "Сенсор", "ОС", "Источник ЦП"};

struct Opt { const char* name; double cost; double q; };
const Opt SIZE_OPT[4] = {{"Компактный", 0.85, 0}, {"Стандарт", 1.0, 0}, {"Большой", 1.2, 0}, {"Огромный", 1.45, 0}};
const Opt COLOR_OPT[8] = {{"Чёрный", 0, 8}, {"Бежевый", 0, 8}, {"Хром", 2, 11}, {"Синий", 1, 10},
                          {"Красный", 1, 10}, {"Зелёный", 1, 10}, {"Кремовый", 4, 14}, {"Золотой", 6, 16}};
const Col COLOR_RGB[8] = {{38, 40, 46, 255},  {230, 226, 210, 255}, {176, 182, 192, 255}, {50, 100, 210, 255},
                          {200, 50, 60, 255}, {50, 160, 90, 255},   {236, 226, 200, 255}, {220, 180, 70, 255}};
const Opt SCREEN_OPT[6] = {{"Нет экрана", 0, 0}, {"Неоновая панель", 15, 10}, {"ЭЛТ монохром", 28, 20},
                           {"ЭЛТ цветной", 48, 32}, {"ЖК монохром", 75, 45}, {"ЖК цветной", 110, 59}};
const Opt CPU_OPT[6] = {{"Электронные лампы", 10, 8}, {"Транзисторы", 22, 18}, {"Интегральная схема", 40, 30},
                        {"Микропроцессор 8-бит", 68, 44}, {"Микропроцессор 32-бит", 105, 59}, {"Квантовый чип", 155, 75}};
const Opt BATT_OPT[6] = {{"Сухие элементы", 8, 8}, {"Никель-кадмиевая", 15, 18}, {"Никель-металлгидридная", 26, 29},
                         {"Литий-ионная", 40, 41}, {"Литий-полимерная", 60, 55}, {"Твердотельная", 88, 70}};
const Opt SPK_OPT[5] = {{"Громкоговоритель", 4, 6}, {"Стерео", 9, 15}, {"Hi-Fi стерео", 18, 26},
                        {"Объёмный звук", 30, 39}, {"Студийный", 46, 53}};
const Opt PACK_OPT[5] = {{"Бумажный пакет", 1, 2}, {"Картонная коробка", 3, 7}, {"Пластиковый блистер", 5, 12},
                         {"Эко-коробка", 9, 16}, {"Премиум-коробка", 16, 22}};

const Opt CAMN_OPT[5] = {{"Без камер", 0, 0}, {"1 камера", 12, 12}, {"2 камеры", 26, 22},
                          {"3 камеры", 40, 30}, {"4 камеры", 58, 38}};
const Opt CAMRES_OPT[6] = {{"Без камеры", 0, 0}, {"0.3 Мп VGA", 6, 8}, {"1.3 Мп", 12, 14},
                           {"5 Мп", 24, 24}, {"12 Мп", 40, 36}, {"48 Мп", 70, 50}};
const Opt CAMSZ_OPT[4] = {{"Миниатюрный", 2, 0}, {"Стандартный", 6, 4}, {"Крупный сенсор", 14, 10},
                          {"Полнокадровый", 30, 18}};

// 0 — своя ОС (разработка за свой счёт), 1..5 — лицензии на готовые системы по годам
// Варианты ОС и процессора: 0 — без лицензии/своего, 1 — своя (после проекта), далее лицензии по годам.
// Значения q у «своих» обновляются, когда проект завершён.
Opt OS_OPT[7] = {{"Без ОС", 0, 0}, {"Своя ОС", 0, 0}, {"Лицензия: UNIX-подобная", 4, 14},
                 {"Лицензия: DOS-подобная", 8, 20}, {"Лицензия: оконная ОС", 14, 28},
                 {"Лицензия: мобильная ОС", 18, 36}, {"Лицензия: смартфонная ОС", 25, 48}};
Opt CPUSRC_OPT[6] = {{"Без чипа", 0, 0}, {"Свой процессор", 0, 0}, {"Лицензия: 16-бит ЦП", 6, 10},
                     {"Лицензия: 32-бит ЦП", 12, 18}, {"Лицензия: мобильный SoC", 18, 30},
                     {"Лицензия: ИИ-ускоритель", 30, 44}};

int optCount(int p) {
    switch (p) {
        case P_SIZE: return 4;
        case P_COLOR: return 8;
        case P_SCREEN: case P_CPU: case P_BATT: return 6;
        case P_CAMN: return 5;
        case P_CAMRES: return 6;
        case P_CAMSZ: return 4;
        case P_OS: return 7;
        case P_CPUSRC: return 6;
        default: return 5;
    }
}
const Opt& opt(int p, int i) {
    switch (p) {
        case P_SIZE: return SIZE_OPT[i];
        case P_COLOR: return COLOR_OPT[i];
        case P_SCREEN: return SCREEN_OPT[i];
        case P_CPU: return CPU_OPT[i];
        case P_BATT: return BATT_OPT[i];
        case P_SPK: return SPK_OPT[i];
        case P_CAMN: return CAMN_OPT[i];
        case P_CAMRES: return CAMRES_OPT[i];
        case P_CAMSZ: return CAMSZ_OPT[i];
        case P_OS: return OS_OPT[i];
        case P_CPUSRC: return CPUSRC_OPT[i];
        default: return PACK_OPT[i];
    }
}

struct Cat { const char* name; const char* series; double costMult; double demand; int idealSize; int defPrice; int devDays; double w[NPARAM]; };
const int NCAT = 5;
const Cat CATS[NCAT] = {
    {"Радиоприёмник", "Радио", 0.8, 50, 1, 45, 16, {0.8, 0.6, 0.0, 0.8, 1.0, 2.0, 0.5, 0, 0, 0, 0, 0.8}},
    {"Телефон", "Тел", 1.2, 40, 1, 79, 20, {1.0, 0.7, 1.5, 1.5, 1.2, 0.8, 0.5, 0.8, 1.2, 0.8, 1.5, 1.5}},
    {"Наручные часы", "Часы", 1.0, 28, 0, 120, 16, {1.0, 1.0, 1.2, 0.8, 1.5, 0.3, 0.5, 0, 0, 0, 0.4, 0.8}},
    {"Калькулятор", "Калк", 1.5, 20, 1, 150, 20, {0.8, 0.5, 1.0, 1.2, 1.0, 0.0, 0.5, 0, 0, 0, 0, 1.2}},
    {"Компьютер", "ЭВМ", 4.0, 14, 2, 1500, 30, {0.9, 0.5, 1.4, 2.0, 1.4, 0.7, 0.5, 0.6, 0.6, 0.5, 2.0, 2.0}},
};
const char* SUFFIX[10] = {"Альфа", "Бета", "Гамма", "Дельта", "Эпсилон", "Омега", "Супер", "Мини", "Модель 1", "Модель 2"};

struct Market { const char* name; double size; double cost; double rep; };
const int NMARKET = 10;
const Market MARKETS[NMARKET] = {
    {"Россия", 1.0, 0, 0},           {"Казахстан", 0.4, 15000, 3},   {"Беларусь", 0.35, 15000, 5},
    {"Германия", 1.2, 150000, 18},   {"Франция", 1.0, 180000, 25},   {"Бразилия", 1.1, 300000, 35},
    {"Индия", 2.0, 600000, 45},      {"Япония", 1.6, 900000, 55},    {"США", 2.8, 2000000, 65},
    {"Китай", 3.2, 3000000, 75},
};

struct Camp { const char* name; double cost; double hype; };
const Camp CAMPS[4] = {{"Посты в соцсетях", 4000, 0.15}, {"Обзоры у блогеров", 15000, 0.4},
                       {"Реклама на ТВ", 50000, 0.9},     {"Глобальная кампания", 180000, 1.8}};

struct Pres { const char* name; double cost; double bonus; double hype; };
const Pres PRES[4] = {{"Без презентации", 0, 0, 0}, {"Пресс-релиз", 3000, 0.3, 0.05},
                      {"Презентация в зале", 20000, 0.7, 0.15}, {"Мега-шоу на стадионе", 90000, 1.2, 0.3}};

const int NOFFICE = 5;
const char* OFFICE_NAME[NOFFICE] = {"Гараж", "Небольшой офис", "Бизнес-центр", "Технопарк", "Штаб-квартира"};
const int OFFICE_CAP[NOFFICE] = {2, 5, 10, 20, 40};
const double OFFICE_MULT[NOFFICE] = {1.0, 1.1, 1.25, 1.45, 1.7};
const double OFFICE_COST[NOFFICE] = {0, 30000, 120000, 400000, 1200000};
const double OFFICE_RENT[NOFFICE] = {0, 2000, 7000, 20000, 50000};

const char* ROLE_NAME[3] = {"Дизайнер", "Программист", "Инженер"};
const char* FIRST_NAMES[16] = {"Алексей", "Мария", "Иван",   "Ольга", "Дмитрий", "Анна",  "Сергей", "Елена",
                               "Никита",  "Юлия",  "Артём",  "Дарья", "Павел",   "Ксения", "Игорь", "Вера"};
const char* INITIALS[12] = {"А.", "Б.", "В.", "Г.", "Д.", "К.", "Л.", "М.", "Н.", "П.", "С.", "Т."};

const int NCOMP = 5;
const char* COMP_NAMES[NCOMP] = {"Грушафон", "СуперЛаб", "Восток-Электро", "НеоТек", "ПиксельКо"};
const char* MONTHS[12] = {"Январь", "Февраль", "Март",     "Апрель",  "Май",    "Июнь",
                          "Июль",   "Август",  "Сентябрь", "Октябрь", "Ноябрь", "Декабрь"};
const char* MONTHS_S[12] = {"Янв", "Фев", "Мар", "Апр", "Май", "Июн", "Июл", "Авг", "Сен", "Окт", "Ноя", "Дек"};
const char* OUTLETS[4] = {"ТехноОбзор", "Гаджет-Мир", "Цифра", "Девайс-Ревью"};
const char* COMPANY_NAMES[10] = {"Нова", "Кварц", "Ионика", "ТехноЛис", "Вектор",
                                 "Альфа-Девайс", "Спектр", "Орбита", "Микрон", "Гелиос"};

// Исследования
struct Tech { std::string name; int kind; int a; int b; double cost; int prereq; int year = 0; };
std::vector<Tech> TECHS;
int techFor[NPARAM][8];
int techCat[NCAT];

// Год появления каждого варианта компонента (реальная хронология)
const int PARAM_YEAR[NPARAM][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1960, 1962, 1968, 1976, 1985, 0, 0},   // экран
    {0, 1960, 1965, 1974, 1985, 2035, 0, 0},   // процессор
    {0, 1963, 1989, 1991, 2000, 2025, 0, 0},   // батарея
    {0, 1961, 1968, 1992, 2005, 0, 0, 0},      // динамики
    {0, 1960, 1965, 2005, 2015, 0, 0, 0},      // упаковка
    {0, 2000, 2016, 2019, 2021, 0, 0, 0},      // камеры: количество
    {0, 2001, 2003, 2006, 2014, 2019, 0, 0},   // камеры: разрешение
    {0, 2003, 2014, 2019, 0, 0, 0, 0},         // камеры: размер сенсора
    {0, 0, 1971, 1981, 1985, 2000, 2008, 0},   // ОС: своя и лицензии
    {0, 0, 1978, 1985, 2007, 2017, 0, 0},      // источник ЦП: свой и лицензии
};

void buildTechs() {
    TECHS.clear();
    for (int p = 0; p < NPARAM; ++p) for (int t = 0; t < 8; ++t) techFor[p][t] = -1;
    for (int c = 0; c < NCAT; ++c) techCat[c] = -1;
    auto add = [](const std::string& n, int kind, int a, int b, double cost, int pre) {
        Tech t; t.name = n; t.kind = kind; t.a = a; t.b = b; t.cost = cost; t.prereq = pre;
        TECHS.push_back(t);
        return (int)TECHS.size() - 1;
    };
    techCat[2] = add("Категория: Наручные часы", 1, 2, 0, 220, -1);
    TECHS[(size_t)techCat[2]].year = 1969;
    techCat[3] = add("Категория: Калькулятор", 1, 3, 0, 380, -1);
    TECHS[(size_t)techCat[3]].year = 1972;
    techCat[4] = add("Категория: Компьютер", 1, 4, 0, 650, -1);
    TECHS[(size_t)techCat[4]].year = 1981;
    const int params[8] = {P_SCREEN, P_CPU, P_BATT, P_SPK, P_PACK, P_CAMN, P_CAMRES, P_CAMSZ};
    for (int k = 0; k < 8; ++k) {
        int p = params[k];
        int prev = -1;
        double mul = (p == P_SPK || p == P_PACK) ? 0.6 : 1.0;
        for (int t = 1; t < optCount(p); ++t) {
            prev = add(std::string(PARAM_NAMES[p]) + ": " + opt(p, t).name, 0, p, t, (75.0 * t * t + 60.0 * t) * mul, prev);
            techFor[p][t] = prev;
            TECHS[(size_t)prev].year = PARAM_YEAR[p][t];
        }
    }
    int a1 = add("Автоматизация завода I", 2, 0, 0, 450, -1);
    add("Автоматизация завода II", 2, 0, 0, 1100, a1);
    int d1 = add("Agile-разработка I", 3, 0, 0, 400, -1);
    add("Agile-разработка II", 3, 0, 0, 1000, d1);
    // Технологии, которые появляются в реальные годы
    struct YT { const char* n; int kind; double cost; int year; };
    const YT yearly[] = {
        {"Автоматизированное проектирование (САПР)", 3, 320, 1964},
        {"Роботизированный завод", 2, 600, 1971},
        {"Нейросети: обратное распространение", 4, 900, 1986},
        {"Облачные вычисления", 3, 1100, 2006},
        {"Нейросети: голосовой ассистент", 4, 1600, 2011},
        {"Нейросети: распознавание изображений", 4, 1800, 2012},
        {"Самообучающийся контроль качества", 2, 2000, 2016},
        {"Нейросети: генерация текста", 4, 2400, 2020},
        {"Нейросети: генеративные модели для дизайна", 4, 2800, 2023},
        {"Квантовая разработка", 3, 3500, 2030},
        {"Нейроморфные чипы", 2, 3800, 2035},
    };
    for (const YT& y : yearly) {
        int id = add(y.n, y.kind, 0, 0, y.cost, -1);
        TECHS[(size_t)id].year = y.year;
    }
}

struct Staff { std::string name; int role = 0; int skill = 1; int salary = 1000; };
struct Draft { int cat = 0; int p[NPARAM] = {1, 0, 0, 0, 0, 0, 0}; int price = 199; int suffix = 0; };
struct Product {
    std::string name;
    int cat = 0;
    int p[NPARAM] = {0, 0, 0, 0, 0, 0, 0};
    int price = 0;
    double unitCost = 0, quality = 0, rating = 0;
    int pres = 0;
    int launchDay = 0;
    bool onSale = true;
    double units = 0, revenue = 0, profit = 0, weekAcc = 0, carry = 0, lastDay = 0;
    std::vector<int> weekly;
};

struct Game {
    std::string company = "Девайс-Корп";
    double money = 150000, rep = 5, fans = 0;
    int day = 0, speed = 1, office = 0;
    std::vector<Staff> staff, cand;
    std::vector<char> techDone;
    int researching = -1;
    double researchProg = 0;
    bool market[NMARKET] = {true, false, false, false, false, false, false, false, false, false};
    double hype = 0;
    int trend = 3, negMonths = 0;
    double monInc = 0, monExp = 0;
    std::vector<double> histInc, histExp;
    double todayUnits = 0, todayRev = 0;
    std::vector<Product> products;
    Draft draft, dev;
    bool inDev = false, devReady = false;
    double devProg = 0, devTotal = 1;
    std::string devName;
    int gen[NCAT] = {0, 0, 0, 0, 0};
    double comp[NCOMP] = {900, 650, 420, 260, 140};
    std::vector<std::string> log;
};
Game G;

// ---------------------------------------------------------------------------
//  Экранное состояние
// ---------------------------------------------------------------------------
enum Screen { SC_NEW, SC_MAIN, SC_DESIGN, SC_LAUNCH, SC_OFFICE, SC_RESEARCH, SC_MARKET, SC_STATS, SC_PAUSE, SC_OVER, SC_PLATFORM };
Screen screen = SC_NEW, retScreen = SC_MAIN;
std::string toastMsg;
float toastT = 0;
int selProduct = -1;
int resIdx = -1;
float resT = 0;
double resOutlet[4] = {0, 0, 0, 0};
double resRepDelta = 0;
int presChoice = 1;
int statsTab = 0;
std::string newName;
int capChoice = 1;
bool textActive = false;
bool running = true;
std::string savePath;
bool saveExists = false;
float dayTimer = 0;
float uiTime = 0;

void setScreen(Screen s) {
    if (s == SC_PAUSE && screen != SC_PAUSE) retScreen = screen;
    screen = s;
    U.reset = true;
}
void toast(const std::string& s) { toastMsg = s; toastT = 3.0f; }
void logMsg(const std::string& s) {
    std::string d = std::string(MONTHS_S[(G.day / 30) % 12]) + " " + std::to_string(START_YEAR + G.day / 360) + ": ";
    G.log.push_back(d + s);
    if (G.log.size() > 40) G.log.erase(G.log.begin());
}

// ---------------------------------------------------------------------------
//  Формулы
// ---------------------------------------------------------------------------
bool techDone(int i) { return i >= 0 && i < (int)G.techDone.size() && G.techDone[(size_t)i]; }
// Разработка своей платформы по этапам
struct PO { const char* n; double cost; int days; int q; };
struct Proj { int st = 0; int pick = -1; double prog = 0; int q = 0; };  // st: 0..2 этапы, 3 — готово
Proj PRJ_CPU, PRJ_OS;
const PO CPU_STAGES[3][3] = {
    {{"RISC-подобная", 20000, 30, 4}, {"CISC-подобная", 15000, 40, 3}, {"Сверхскалярная", 45000, 60, 7}},
    {{"Техпроцесс 5 мкм", 10000, 20, 2}, {"Техпроцесс 3 мкм", 30000, 40, 5}, {"Техпроцесс 1 мкм", 70000, 70, 9}},
    {{"Быстрая проверка", 5000, 10, 1}, {"Полный цикл", 25000, 40, 5}, {"Ручная доводка", 60000, 80, 10}}};
const char* const CPU_NAMES[3] = {"Архитектура", "Техпроцесс", "Тестирование"};
const PO OS_STAGES[3][3] = {
    {{"Монолитное ядро", 15000, 30, 4}, {"Микроядро", 30000, 50, 7}, {"Однопользовательская", 5000, 15, 2}},
    {{"Командная строка", 5000, 10, 2}, {"Оконный интерфейс", 40000, 60, 8}, {"Сенсорный интерфейс", 70000, 80, 12}},
    {{"Минимум драйверов", 5000, 10, 1}, {"Совместимость с периферией", 25000, 40, 5}, {"Полная библиотека", 50000, 70, 9}}};
const char* const OS_NAMES[3] = {"Ядро", "Интерфейс", "Драйверы и тесты"};

int calYear();
bool tierUnlocked(int p, int t) {
    if (p == P_SIZE || p == P_COLOR) return true;
    if (p == P_OS) {
        if (t == 0) return true;
        if (t == 1) return PRJ_OS.st == 3;
        return calYear() >= PARAM_YEAR[p][t];
    }
    if (p == P_CPUSRC) {
        if (t == 0) return true;
        if (t == 1) return PRJ_CPU.st == 3;
        return calYear() >= PARAM_YEAR[p][t];
    }
    if (t == 0) return true;
    return techDone(techFor[p][t]);
}
bool catUnlocked(int c) { return c < 2 || techDone(techCat[c]); }
int countKind(int kind) {
    int n = 0;
    for (size_t i = 0; i < TECHS.size(); ++i) if (TECHS[i].kind == kind && techDone((int)i)) ++n;
    return n;
}
double skillSum(int role) {
    double s = 0;
    for (size_t i = 0; i < G.staff.size(); ++i) if (G.staff[i].role == role) s += G.staff[i].skill;
    return s;
}
double researchRate() {
    return (5.0 + 0.3 * skillSum(0) + 0.6 * skillSum(1) + 0.45 * skillSum(2)) * OFFICE_MULT[G.office];
}
double devSpeed() {
    return (1.0 + 0.03 * skillSum(2) + 0.02 * skillSum(1) + 0.01 * skillSum(0)) * OFFICE_MULT[G.office] *
           (1.0 + 0.15 * countKind(3)) * (1.0 + 0.08 * countKind(4));
}
double marketSize() {
    double s = 0;
    for (int i = 0; i < NMARKET; ++i) if (G.market[i]) s += MARKETS[i].size;
    return s;
}
int marketsOpen() { int n = 0; for (int i = 0; i < NMARKET; ++i) if (G.market[i]) ++n; return n; }
double years() { return G.day / 360.0; }
double expectation() { return 18.0 + 4.5 * years(); }
double salaries() { double s = 0; for (size_t i = 0; i < G.staff.size(); ++i) s += G.staff[i].salary; return s; }

double draftUnitCost(const Draft& d) {
    const Cat& c = CATS[d.cat];
    double sum = COLOR_OPT[d.p[P_COLOR]].cost;
    for (int p = P_SCREEN; p < NPARAM; ++p) if (c.w[p] > 0) sum += opt(p, d.p[p]).cost;
    return sum * SIZE_OPT[d.p[P_SIZE]].cost * c.costMult * (1.0 - 0.08 * countKind(2));
}
double groupBonus(int role) { return 15.0 * (1.0 - std::exp(-skillSum(role) / 25.0)); }
double draftQuality(const Draft& d) {
    const Cat& c = CATS[d.cat];
    double bD = groupBonus(0), bP = groupBonus(1), bE = groupBonus(2);
    double sw = 0, sq = 0;
    for (int p = 0; p < NPARAM; ++p) {
        double w = c.w[p];
        if (w <= 0) continue;
        double q, b;
        if (p == P_SIZE) { q = std::max(0.0, 20.0 - 9.0 * std::abs(d.p[p] - c.idealSize)); b = bD; }
        else if (p == P_COLOR) { q = COLOR_OPT[d.p[p]].q + (d.p[p] == G.trend ? 10.0 : 0.0); b = bD; }
        else {
            q = opt(p, d.p[p]).q;
            b = (p == P_PACK) ? bD : ((p == P_CPU || p == P_SCREEN) ? bP : bE);
        }
        sw += w; sq += w * (q + b);
    }
    return sw > 0 ? sq / sw : 0;
}
double scoreFromQ(double q) { return clampd(10.0 * (1.0 - std::exp(-0.9 * q / expectation())), 1.0, 10.0); }
double fairPrice(double unitCost, double rating) { return unitCost * (1.25 + 0.08 * rating); }
double draftDevTotal(const Draft& d) {
    int sum = 0;
    for (int p = P_SCREEN; p < NPARAM; ++p) sum += d.p[p];
    return CATS[d.cat].devDays * (1.0 + 0.12 * sum);
}
double toolingCost(const Draft& d) { return draftUnitCost(d) * 150.0 * std::sqrt(marketSize()) + 5000.0; }
std::string draftName(const Draft& d) {
    return std::string(CATS[d.cat].series) + " " + SUFFIX[d.suffix] + " " + std::to_string(G.gen[d.cat] + 1);
}
Col ratingCol(double r) { return r >= 7.5 ? C_GREEN : (r >= 5.0 ? C_YEL : (r >= 3.5 ? C_ORANGE : C_RED)); }
int priceStep(int p) { return p < 100 ? 5 : (p < 500 ? 10 : (p < 2000 ? 25 : 50)); }

void fixDraft(Draft& d) {
    for (int p = 0; p < NPARAM; ++p) {
        d.p[p] = clampi(d.p[p], 0, optCount(p) - 1);
        while (d.p[p] > 0 && !tierUnlocked(p, d.p[p])) --d.p[p];
    }
    if (CATS[d.cat].w[P_SCREEN] <= 0) d.p[P_SCREEN] = 0;
}

Staff genCandidate() {
    Staff s;
    s.role = irand(0, 2);
    s.skill = clampi(irand(1, 4 + G.office * 2), 1, 10);
    s.salary = ((1200 + s.skill * 550 + irand(0, 300)) / 50) * 50;
    s.name = std::string(FIRST_NAMES[irand(0, 15)]) + " " + INITIALS[irand(0, 11)];
    return s;
}
void refreshCandidates() {
    G.cand.clear();
    for (int i = 0; i < 3; ++i) G.cand.push_back(genCandidate());
}

const int START_YEAR = 1960;  // игра начинается в 1960 году
int calYear() { return START_YEAR + G.day / 360; }

void newGame(const std::string& name, int capIdx) {
    G = Game();
    PRJ_CPU = Proj();
    PRJ_OS = Proj();
    OS_OPT[1].q = 0;
    CPUSRC_OPT[1].q = 0;
    G.company = name.empty() ? "Девайс-Корп" : name;
    const double caps[3] = {500000, 150000, 50000};
    G.money = caps[clampi(capIdx, 0, 2)];
    G.rep = capIdx == 2 ? 0 : 5;
    G.techDone.assign(TECHS.size(), 0);
    G.trend = irand(0, 7);
    refreshCandidates();
    G.draft.cat = 0;
    G.draft.p[P_SIZE] = CATS[0].idealSize;
    G.draft.price = CATS[0].defPrice;
    logMsg("Компания «" + G.company + "» основана!");
    logMsg("Модный цвет года: " + std::string(COLOR_OPT[G.trend].name));
    selProduct = -1;
    resIdx = -1;
    dayTimer = 0;
}

// ---------------------------------------------------------------------------
//  Симуляция
// ---------------------------------------------------------------------------
void monthEnd() {
    double cost = salaries() + OFFICE_RENT[G.office];
    G.money -= cost;
    G.monExp += cost;
    G.histInc.push_back(G.monInc);
    G.histExp.push_back(G.monExp);
    if (G.histInc.size() > 36) { G.histInc.erase(G.histInc.begin()); G.histExp.erase(G.histExp.begin()); }
    G.monInc = 0; G.monExp = 0;
    refreshCandidates();
    G.rep = std::max(0.0, G.rep * 0.985);
    for (int i = 0; i < NCOMP; ++i) G.comp[i] *= 1.0 + 0.01 + 0.02 * frand();
    if (G.money < 0) {
        G.negMonths++;
        if (G.negMonths >= 3) {
            logMsg("Компания обанкротилась.");
            sfx(SFX_BAD);
            setScreen(SC_OVER);
        } else {
            logMsg("Деньги в минусе! До банкротства: " + std::to_string(3 - G.negMonths) + " мес.");
            toast("Внимание: отрицательный баланс!");
            sfx(SFX_ERR);
        }
    } else {
        G.negMonths = 0;
    }
}

void announceYear(int y) {
    std::string first;
    int cnt = 0;
    for (size_t i = 0; i < TECHS.size(); ++i)
        if (TECHS[i].year == START_YEAR + y) { if (cnt == 0) first = TECHS[i].name; ++cnt; }
    logMsg("Наступил " + std::to_string(START_YEAR + y) + " год. Новых технологий: " + std::to_string(cnt));
    if (cnt > 0) toast("Новое в мире: " + first);
    sfx(SFX_DONE);
}

void advanceProj(Proj& pr, const PO (&st)[3][3], Opt& own, const char* const* names, const char* what) {
    if (pr.st >= 3 || pr.pick < 0) return;
    pr.prog += devSpeed();
    const PO& o = st[pr.st][pr.pick];
    if (pr.prog < o.days) return;
    pr.q += o.q;
    own.q = pr.q;
    pr.st++;
    pr.pick = -1;
    pr.prog = 0;
    if (pr.st == 3) {
        logMsg(std::string(what) + " готов. Качество: " + std::to_string(pr.q));
        toast(std::string(what) + " готов!");
        sfx(SFX_DONE);
    } else {
        logMsg(std::string(what) + ": этап «" + names[pr.st - 1] + "» завершён");
    }
}

void tickProjects() {
    advanceProj(PRJ_CPU, CPU_STAGES, CPUSRC_OPT[1], CPU_NAMES, "Процессор");
    advanceProj(PRJ_OS, OS_STAGES, OS_OPT[1], OS_NAMES, "Операционная система");
}

void tickDay() {
    G.day++;
    if (G.day > 0 && G.day % 360 == 0) announceYear(G.day / 360);
    if (G.researching >= 0) {
        G.researchProg += researchRate();
        const Tech& t = TECHS[(size_t)G.researching];
        if (G.researchProg >= t.cost) {
            G.techDone[(size_t)G.researching] = 1;
            logMsg("Исследование завершено: " + t.name);
            toast("Изучено: " + t.name);
            sfx(SFX_DONE);
            G.researching = -1;
            G.researchProg = 0;
            fixDraft(G.draft);
        }
    }
    tickProjects();
    if (G.inDev && !G.devReady) {
        G.devProg += devSpeed();
        if (G.devProg >= G.devTotal) {
            G.devReady = true;
            logMsg("Разработка «" + G.devName + "» завершена. Пора запускать!");
            toast("«" + G.devName + "» готов к запуску!");
            sfx(SFX_DONE);
        }
    }
    G.todayUnits = 0;
    G.todayRev = 0;
    double ms = marketSize();
    int sameCat[NCAT] = {0, 0, 0, 0, 0};
    for (size_t i = 0; i < G.products.size(); ++i) if (G.products[i].onSale) sameCat[G.products[i].cat]++;
    for (size_t i = 0; i < G.products.size(); ++i) {
        Product& p = G.products[i];
        if (!p.onSale) continue;
        int age = G.day - p.launchDay;
        double life = std::exp(-age / (60.0 + p.rating * 15.0)) * (age < 10 ? 1.6 : 1.0);
        double ratingF = std::pow(std::max(p.rating, 0.5) / 5.0, 1.6);
        double fair = fairPrice(p.unitCost, p.rating);
        double priceF = clampd(std::pow(fair / std::max(1, p.price), 2.5), 0.02, 3.0);
        double repF = 0.6 + G.rep / 100.0 * 0.8;
        double demand = CATS[p.cat].demand * ms * ratingF * priceF * repF * (1.0 + 0.5 * G.hype) * life /
                        (double)std::max(1, sameCat[p.cat]);
        demand *= 0.85 + 0.3 * frand();
        double u = p.carry + demand;
        double whole = std::floor(u);
        p.carry = u - whole;
        double rev = whole * p.price, cost = whole * p.unitCost;
        p.units += whole; p.revenue += rev; p.profit += rev - cost; p.lastDay = whole; p.weekAcc += whole;
        G.money += rev - cost;
        G.monInc += rev;
        G.monExp += cost;
        G.todayUnits += whole;
        G.todayRev += rev;
        G.fans += whole * 0.3;
        if (age % 7 == 0) {
            p.weekly.push_back((int)p.weekAcc);
            p.weekAcc = 0;
            if (p.weekly.size() > 104) p.weekly.erase(p.weekly.begin());
        }
        if (age > 40 && life < 0.04) {
            p.onSale = false;
            logMsg("«" + p.name + "» снят с продаж: спрос иссяк.");
        }
    }
    G.hype *= 0.98;
    if (irand(0, 39) == 0) {
        int c = irand(0, NCOMP - 1);
        double r = clampd(5.0 + 4.0 * frand() + years() * 0.1, 1, 10);
        G.comp[c] *= 1.03;
        logMsg(std::string(COMP_NAMES[c]) + " выпустила новинку (оценка " + fmt1(r) + ")");
    }
    if (G.day % 30 == 0) monthEnd();
    if (G.day % 360 == 0) {
        G.trend = irand(0, 7);
        logMsg("Новый год! Модный цвет: " + std::string(COLOR_OPT[G.trend].name));
        toast("Новый год! Ожидания рынка выросли.");
    }
}

void launchProduct() {
    Product p;
    p.name = G.devName;
    p.cat = G.dev.cat;
    for (int i = 0; i < NPARAM; ++i) p.p[i] = G.dev.p[i];
    p.price = G.dev.price;
    p.unitCost = draftUnitCost(G.dev);
    p.quality = draftQuality(G.dev);
    p.pres = presChoice;
    double r = scoreFromQ(p.quality) + PRES[presChoice].bonus + (frand() - 0.5) * 0.4;
    if (p.price > fairPrice(p.unitCost, r) * 1.6) r -= 0.5;
    p.rating = clampd(std::round(r * 10.0) / 10.0, 1.0, 10.0);
    p.launchDay = G.day;
    double pc = PRES[presChoice].cost * (0.7 + 0.3 * marketSize());
    G.money -= pc;
    G.monExp += pc;
    G.hype = std::min(4.0, G.hype + PRES[presChoice].hype);
    double oldRep = G.rep;
    double dRep = (p.rating - 5.0) * 1.5;
    if (dRep > 0) dRep *= 1.0 - G.rep / 110.0;
    G.rep = clampd(G.rep + dRep, 0, 100);
    resRepDelta = G.rep - oldRep;
    G.fans += p.rating * 60.0;
    for (int i = 0; i < 4; ++i) resOutlet[i] = clampd(std::round((p.rating + (frand() - 0.5) * 1.2) * 10) / 10, 1, 10);
    G.products.push_back(p);
    G.inDev = false;
    G.devReady = false;
    resIdx = (int)G.products.size() - 1;
    selProduct = resIdx;
    resT = 0;
    logMsg("Запуск «" + p.name + "»: оценка " + fmt1(p.rating) + "/10");
    sfx(p.rating >= 5 ? SFX_LAUNCH : SFX_BAD);
}

// ---------------------------------------------------------------------------
//  Сохранение
// ---------------------------------------------------------------------------
void wstr(std::ostringstream& o, const std::string& s) { o << '\n' << s << '\n'; }
bool rstr(std::istringstream& is, std::string& s) {
    is >> std::ws;
    return (bool)std::getline(is, s);
}

bool saveGame() {
    if (savePath.empty()) return false;
    std::ostringstream o;
    o.precision(17);
    o << "DEVCORP1";
    wstr(o, G.company);
    o << G.money << ' ' << G.rep << ' ' << G.fans << ' ' << G.day << ' ' << G.speed << ' ' << G.office << ' '
      << G.hype << ' ' << G.trend << ' ' << G.negMonths << ' ' << G.monInc << ' ' << G.monExp << ' '
      << G.researching << ' ' << G.researchProg << ' ' << A.vol << '\n';
    o << G.techDone.size();
    for (size_t i = 0; i < G.techDone.size(); ++i) o << ' ' << (int)G.techDone[i];
    o << '\n';
    for (int i = 0; i < NMARKET; ++i) o << (G.market[i] ? 1 : 0) << ' ';
    for (int i = 0; i < NCAT; ++i) o << G.gen[i] << ' ';
    for (int i = 0; i < NCOMP; ++i) o << G.comp[i] << ' ';
    o << '\n';
    const std::vector<Staff>* lists[2] = {&G.staff, &G.cand};
    for (int l = 0; l < 2; ++l) {
        o << lists[l]->size() << '\n';
        for (size_t i = 0; i < lists[l]->size(); ++i) {
            const Staff& s = (*lists[l])[i];
            o << s.role << ' ' << s.skill << ' ' << s.salary;
            wstr(o, s.name);
        }
    }
    const Draft* ds[2] = {&G.draft, &G.dev};
    for (int k = 0; k < 2; ++k) {
        o << ds[k]->cat << ' ' << ds[k]->price << ' ' << ds[k]->suffix;
        for (int p = 0; p < NPARAM; ++p) o << ' ' << ds[k]->p[p];
        o << '\n';
    }
    o << (G.inDev ? 1 : 0) << ' ' << (G.devReady ? 1 : 0) << ' ' << G.devProg << ' ' << G.devTotal;
    wstr(o, G.devName.empty() ? std::string("-") : G.devName);
    o << G.histInc.size();
    for (size_t i = 0; i < G.histInc.size(); ++i) o << ' ' << G.histInc[i] << ' ' << G.histExp[i];
    o << '\n' << G.products.size() << '\n';
    for (size_t i = 0; i < G.products.size(); ++i) {
        const Product& p = G.products[i];
        o << p.cat << ' ' << p.price << ' ' << p.unitCost << ' ' << p.quality << ' ' << p.rating << ' ' << p.pres
          << ' ' << p.launchDay << ' ' << (p.onSale ? 1 : 0) << ' ' << p.units << ' ' << p.revenue << ' '
          << p.profit << ' ' << p.weekAcc << ' ' << p.carry << ' ' << p.lastDay;
        for (int k = 0; k < NPARAM; ++k) o << ' ' << p.p[k];
        o << ' ' << p.weekly.size();
        for (size_t k = 0; k < p.weekly.size(); ++k) o << ' ' << p.weekly[k];
        wstr(o, p.name);
    }
    o << G.log.size() << '\n';
    for (size_t i = 0; i < G.log.size(); ++i) o << G.log[i] << '\n';
    std::string data = o.str();
    SDL_RWops* f = SDL_RWFromFile(savePath.c_str(), "wb");
    if (!f) return false;
    size_t wr = SDL_RWwrite(f, data.data(), 1, data.size());
    SDL_RWclose(f);
    if (wr == data.size()) saveExists = true;
    return wr == data.size();
}

bool loadGame() {
    if (savePath.empty()) return false;
    SDL_RWops* f = SDL_RWFromFile(savePath.c_str(), "rb");
    if (!f) return false;
    Sint64 sz = SDL_RWsize(f);
    if (sz <= 0 || sz > 50 * 1024 * 1024) { SDL_RWclose(f); return false; }
    std::string data((size_t)sz, '\0');
    size_t rd = SDL_RWread(f, &data[0], 1, (size_t)sz);
    SDL_RWclose(f);
    if (rd != (size_t)sz) return false;
    std::istringstream is(data);
    std::string magic;
    is >> magic;
    if (magic != "DEVCORP1") return false;
    Game g;
    float vol = A.vol;
    if (!rstr(is, g.company)) return false;
    is >> g.money >> g.rep >> g.fans >> g.day >> g.speed >> g.office >> g.hype >> g.trend >> g.negMonths >> g.monInc >>
        g.monExp >> g.researching >> g.researchProg >> vol;
    size_t nt = 0;
    is >> nt;
    if (!is || nt != TECHS.size()) return false;
    g.techDone.assign(nt, 0);
    for (size_t i = 0; i < nt; ++i) { int v = 0; is >> v; g.techDone[i] = (char)(v ? 1 : 0); }
    for (int i = 0; i < NMARKET; ++i) { int v = 0; is >> v; g.market[i] = v != 0; }
    for (int i = 0; i < NCAT; ++i) is >> g.gen[i];
    for (int i = 0; i < NCOMP; ++i) is >> g.comp[i];
    std::vector<Staff>* lists[2] = {&g.staff, &g.cand};
    for (int l = 0; l < 2; ++l) {
        size_t n = 0;
        is >> n;
        if (!is || n > 1000) return false;
        for (size_t i = 0; i < n; ++i) {
            Staff s;
            is >> s.role >> s.skill >> s.salary;
            if (!rstr(is, s.name)) return false;
            s.role = clampi(s.role, 0, 2);
            lists[l]->push_back(s);
        }
    }
    Draft* ds[2] = {&g.draft, &g.dev};
    for (int k = 0; k < 2; ++k) {
        is >> ds[k]->cat >> ds[k]->price >> ds[k]->suffix;
        for (int p = 0; p < NPARAM; ++p) is >> ds[k]->p[p];
        ds[k]->cat = clampi(ds[k]->cat, 0, NCAT - 1);
        ds[k]->suffix = clampi(ds[k]->suffix, 0, 9);
        for (int p = 0; p < NPARAM; ++p) ds[k]->p[p] = clampi(ds[k]->p[p], 0, optCount(p) - 1);
    }
    int inDev = 0, devReady = 0;
    is >> inDev >> devReady >> g.devProg >> g.devTotal;
    g.inDev = inDev != 0; g.devReady = devReady != 0;
    if (!rstr(is, g.devName)) return false;
    size_t nh = 0;
    is >> nh;
    if (!is || nh > 1000) return false;
    for (size_t i = 0; i < nh; ++i) {
        double a = 0, b = 0;
        is >> a >> b;
        g.histInc.push_back(a); g.histExp.push_back(b);
    }
    size_t np = 0;
    is >> np;
    if (!is || np > 100000) return false;
    for (size_t i = 0; i < np; ++i) {
        Product p;
        int onSale = 0;
        is >> p.cat >> p.price >> p.unitCost >> p.quality >> p.rating >> p.pres >> p.launchDay >> onSale >> p.units >>
            p.revenue >> p.profit >> p.weekAcc >> p.carry >> p.lastDay;
        p.onSale = onSale != 0;
        p.cat = clampi(p.cat, 0, NCAT - 1);
        p.pres = clampi(p.pres, 0, 3);
        for (int k = 0; k < NPARAM; ++k) { is >> p.p[k]; p.p[k] = clampi(p.p[k], 0, optCount(k) - 1); }
        size_t nw = 0;
        is >> nw;
        if (!is || nw > 10000) return false;
        for (size_t k = 0; k < nw; ++k) { int v = 0; is >> v; p.weekly.push_back(v); }
        if (!rstr(is, p.name)) return false;
        g.products.push_back(p);
    }
    size_t nl = 0;
    is >> nl;
    if (!is || nl > 1000) return false;
    for (size_t i = 0; i < nl; ++i) {
        std::string s;
        if (!rstr(is, s)) break;
        g.log.push_back(s);
    }
    g.office = clampi(g.office, 0, NOFFICE - 1);
    g.trend = clampi(g.trend, 0, 7);
    g.speed = clampi(g.speed, 0, 3);
    if (g.researching >= (int)TECHS.size()) g.researching = -1;
    G = g;
    A.vol = (float)clampd(vol, 0, 1);
    selProduct = G.products.empty() ? -1 : 0;
    resIdx = -1;
    dayTimer = 0;
    return true;
}

// ============================================================================
//  Отрисовка устройства
// ============================================================================
void drawDevice(int cx, int cy, int cat, int size, int color, int scrTier, int camN = 0, int camSz = 0) {
    Col body = COLOR_RGB[color];
    Col edge = shade(body, 0.7);
    Col dark = {18, 22, 30, 255};
    Col scrC = {(Uint8)(20 + scrTier * 10), (Uint8)(40 + scrTier * 22), (Uint8)(80 + scrTier * 28), 255};
    double s = 0.85 + 0.12 * size;
    static const int catShape[5] = {3, 0, 2, 3, 4};  // радио и калькулятор — короб, телефон — вертикальный
    switch (catShape[cat]) {
        case 0: {
            int w = (int)(80 * s), h = (int)(160 * s);
            SDL_Rect b = {cx - w / 2, cy - h / 2, w, h};
            fillRound({b.x - 2, b.y - 2, b.w + 4, b.h + 4}, 14, edge);
            fillRound(b, 12, body);
            fillRound({b.x + 6, b.y + 16, w - 12, h - 32}, 6, scrC);
            fillRect(b.x + 10, b.y + 22, (w - 20) / 2, 3, shade(scrC, 1.5));
            fillCircle(cx, b.y + 8, 3, dark);
            fillRound({cx - 12, b.y + h - 11, 24, 5}, 2, edge);
            break;
        }
        case 1: {
            int rad = (int)(70 * s);
            for (int a = 180; a <= 360; a += 4) {
                double ang = a * 3.14159265 / 180.0;
                fillCircle(cx + (int)(std::cos(ang) * rad), cy + (int)(std::sin(ang) * rad), 7, edge);
            }
            for (int a = 180; a <= 360; a += 4) {
                double ang = a * 3.14159265 / 180.0;
                fillCircle(cx + (int)(std::cos(ang) * rad), cy + (int)(std::sin(ang) * rad), 5, body);
            }
            int cw = (int)(38 * s), ch = (int)(74 * s);
            fillRound({cx - rad - cw / 2, cy - 8, cw, ch}, 14, body);
            fillRound({cx + rad - cw / 2, cy - 8, cw, ch}, 14, body);
            fillRound({cx - rad + cw / 2 - 6, cy + 2, 8, ch - 20}, 3, dark);
            fillRound({cx + rad - cw / 2 - 2, cy + 2, 8, ch - 20}, 3, dark);
            break;
        }
        case 2: {
            int w = (int)(96 * s), h = (int)(112 * s);
            fillRect(cx - w / 3, cy - 120, w * 2 / 3, 240, edge);
            for (int i = 0; i < 5; ++i) fillRect(cx - 3, cy + h / 2 + 10 + i * 12, 6, 6, dark);
            fillRound({cx - w / 2, cy - h / 2, w, h}, 18, body);
            fillRound({cx - w / 2 + 8, cy - h / 2 + 8, w - 16, h - 16}, 12, scrC);
            fillRect(cx + w / 2, cy - 10, 6, 20, edge);
            drawTextC("12:00", cx, cy - 7, 2, C_TEXT);
            break;
        }
        case 3: {
            int w = (int)(240 * s), h = (int)(160 * s);
            SDL_Rect b = {cx - w / 2, cy - h / 2, w, h};
            fillRound({b.x - 2, b.y - 2, b.w + 4, b.h + 4}, 16, edge);
            fillRound(b, 14, body);
            fillRound({b.x + 12, b.y + 12, w - 24, h - 24}, 6, scrC);
            fillRect(b.x + 18, b.y + 18, w / 3, 3, shade(scrC, 1.5));
            fillCircle(b.x + w / 2, b.y + 6, 2, dark);
            break;
        }
        default: {
            int w = (int)(250 * s), h = (int)(150 * s);
            SDL_Rect lid = {cx - w / 2, cy - h / 2 - 20, w, h};
            fillRound(lid, 10, body);
            fillRect(lid.x + 10, lid.y + 10, w - 20, h - 20, scrC);
            fillRect(lid.x + 16, lid.y + 16, w / 3, 3, shade(scrC, 1.5));
            int by = lid.y + h;
            fillRect(lid.x - 20, by, w + 40, (int)(14 * s), edge);
            fillRect(lid.x - 20, by, w + 40, 4, body);
            fillRect(cx - 30, by + 4, 60, 4, shade(edge, 0.8));
            break;
        }
    }
    // Камеры: количество и размер линз зависят от настроек
    if (camN > 0) {
        int rad = 4 + camSz * 2;
        for (int i = 0; i < camN; ++i) {
            int x = cx - 30 + i * (2 * rad + 8), y = cy - 62;
            fillCircle(x, y, rad + 2, {60, 60, 70, 255});
            fillCircle(x, y, rad, {20, 40, 70, 255});
            fillCircle(x - rad / 3, y - rad / 3, std::max(1, rad / 4), {120, 170, 230, 255});
        }
    }
}

// ============================================================================
//  Экраны
// ============================================================================
// Пока у компании нет ни одного устройства, нужно создать первое в конструкторе
bool mustDesignFirst() { return G.products.empty() && !G.inDev; }

void goBack() {
    if (textActive) { textActive = false; SDL_StopTextInput(); return; }
    if (screen == SC_DESIGN && mustDesignFirst()) { toast("Сначала создайте первое устройство"); sfx(SFX_ERR); return; }
    switch (screen) {
        case SC_NEW: case SC_OVER: return;
        case SC_MAIN: setScreen(SC_PAUSE); break;
        case SC_PAUSE: setScreen(retScreen); break;
        case SC_PLATFORM: setScreen(retScreen); break;
        case SC_LAUNCH:
            if (resIdx >= 0) resIdx = -1;
            else setScreen(SC_MAIN);
            break;
        default: setScreen(SC_MAIN); break;
    }
    sfx(SFX_BACK);
}

void topBar() {
    fillRect(0, 0, W, 64, C_PANEL2);
    fillRect(0, 63, W, 1, C_BORDER);
    if (screen != SC_MAIN) {
        if (button({8, 8, 130, 48}, "< Назад")) goBack();
        const char* titles[] = {"", "", "Конструктор", "Запуск", "Офис", "Наука", "Рынки", "Статистика", "", "", "Платформа"};
        drawText(titles[screen], 150, 25, 2, C_DIM);
    } else {
        drawText(fitText(G.company, 2, 310), 16, 25, 2, C_ACC);
    }
    {
        std::string m = fmtMoney(G.money);
        int msc = textW(m, 3) > 236 ? 2 : 3;
        drawText(m, 340, msc == 3 ? 18 : 25, msc, G.money < 0 ? C_RED : C_GREEN);
    }
    drawText("Реп. " + std::to_string((int)G.rep), 590, 25, 2, C_YEL);
    drawText(std::string(MONTHS[(G.day / 30) % 12]) + " " + std::to_string(START_YEAR + G.day / 360), 730, 25, 2, C_TEXT);
    const char* sp[4] = {"||", "1x", "2x", "4x"};
    int x = W - 8 - 100 - 8 - 4 * 60;
    for (int i = 0; i < 4; ++i)
        if (button({x + i * 60, 8, 56, 48}, sp[i], true, G.speed == i)) G.speed = i;
    if (button({W - 108, 8, 100, 48}, "Меню", !(screen == SC_DESIGN && mustDesignFirst()))) setScreen(SC_PAUSE);
}

void scrMain() {
    // Финансы
    panel(16, 76, 600, 140, "Финансы");
    drawText("Выручка сегодня: " + fmtMoney(G.todayRev), 32, 108, 2, C_TEXT);
    drawText("Продано сегодня: " + fmtNum(G.todayUnits) + " шт.", 32, 132, 2, C_TEXT);
    double prof = G.monInc - G.monExp;
    drawText("Итог месяца: " + fmtMoney(prof), 32, 156, 2, prof >= 0 ? C_GREEN : C_RED);
    drawText("Зарплаты+аренда: " + fmtMoney(salaries() + OFFICE_RENT[G.office]) + "/мес", 32, 180, 2, C_DIM);
    // Компания
    panel(16, 226, 600, 110, "Компания");
    drawText("Репутация", 32, 258, 2, C_TEXT);
    progressBar({170, 256, 300, 18}, G.rep / 100.0, C_YEL);
    drawText(std::to_string((int)G.rep) + "/100", 484, 258, 2, C_YEL);
    drawText("Фанаты: " + fmtNum(G.fans), 32, 284, 2, C_TEXT);
    drawText("Рынки: " + std::to_string(marketsOpen()) + "/" + std::to_string(NMARKET) + "  (объём x" +
                 fmt1(marketSize()) + ")", 32, 308, 2, C_TEXT);
    // Разработка
    panel(16, 346, 600, 100, "Разработка");
    if (G.devReady) {
        if (button({32, 380, 568, 54}, "«" + G.devName + "» готов! К запуску >", true, true)) setScreen(SC_LAUNCH);
    } else if (G.inDev) {
        drawText(fitText(G.devName, 2, 300), 32, 380, 2, C_TEXT);
        double left = std::max(0.0, (G.devTotal - G.devProg) / devSpeed());
        drawTextR("≈ " + std::to_string((int)std::ceil(left)) + " дн.", 600, 380, 2, C_DIM);
        progressBar({32, 408, 568, 22}, G.devProg / G.devTotal, C_ACC);
    } else {
        drawText("Нет активного проекта", 32, 392, 2, C_DIM);
        if (button({370, 380, 230, 54}, "Создать")) setScreen(SC_DESIGN);
    }
    // Исследования
    panel(16, 456, 600, 96, "Исследование");
    if (G.researching >= 0) {
        const Tech& t = TECHS[(size_t)G.researching];
        drawText(fitText(t.name, 2, 400), 32, 486, 2, C_TEXT);
        double left = std::max(0.0, (t.cost - G.researchProg) / researchRate());
        drawTextR("≈ " + std::to_string((int)std::ceil(left)) + " дн.", 600, 486, 2, C_DIM);
        progressBar({32, 514, 568, 20}, G.researchProg / t.cost, C_GREEN);
    } else {
        drawText("Ничего не изучается", 32, 500, 2, C_DIM);
        if (button({370, 486, 230, 54}, "Выбрать")) setScreen(SC_RESEARCH);
    }
    // Активные продукты
    panel(16, 562, 600, 150, "В продаже");
    int y = 592, shown = 0;
    for (int i = (int)G.products.size() - 1; i >= 0 && shown < 4; --i) {
        const Product& p = G.products[(size_t)i];
        if (!p.onSale) continue;
        drawText(fitText(p.name, 2, 260), 32, y, 2, C_TEXT);
        drawText(fmt1(p.rating), 310, y, 2, ratingCol(p.rating));
        drawTextR(fmtNum(p.lastDay) + " шт/день", 600, y, 2, C_DIM);
        y += 28; ++shown;
    }
    if (!shown) drawText("Пока ничего не продаётся", 32, y, 2, C_DIM);

    // Навигация
    const char* labels[8] = {"Конструктор", "Запуск и продукты", "Офис и штат", "Исследования",
                             "Маркетинг и рынки", "Статистика", "Сохранить", "Пауза и настройки"};
    const Screen targets[8] = {SC_DESIGN, SC_LAUNCH, SC_OFFICE, SC_RESEARCH, SC_MARKET, SC_STATS, SC_MAIN, SC_PAUSE};
    for (int i = 0; i < 8; ++i) {
        SDL_Rect r = {640 + (i % 2) * 316, 76 + (i / 2) * 82, 304, 70};
        if (button(r, labels[i], true, i == 1 && G.devReady)) {
            if (i == 6) toast(saveGame() ? "Игра сохранена" : "Не удалось сохранить");
            else setScreen(targets[i]);
        }
    }
    panel(640, 410, 624, 302, "Новости");
    int ly = 442;
    int start = std::max(0, (int)G.log.size() - 9);
    for (int i = (int)G.log.size() - 1; i >= start; --i) {
        drawText(fitText(G.log[(size_t)i], 2, 592), 656, ly, 2, i == (int)G.log.size() - 1 ? C_TEXT : C_DIM);
        ly += 29;
    }
}

// ----- Иконки: рисуются примитивами, без ассетов -----
// kind 0..4  — категории (телефон, наушники, часы, планшет, ноутбук)
// kind 10..16 — параметры (размер, цвет, экран, процессор, батарея, динамики, упаковка)
// kind 17 — цена, 18 — название
void drawIcon(int kind, int cx, int cy, int S, Col c) {
    static const int catIcon[5] = {20, 0, 2, 19, 4};  // категории -> иконки
    if (kind >= 0 && kind < 5) kind = catIcon[kind];
    const double k = S / 40.0;
    const Col d = shade(c, 0.55);
    const Col scrC = {36, 84, 140, 255};
    auto px = [&](double x) { return cx - S / 2 + (int)(x * k); };
    auto py = [&](double y) { return cy - S / 2 + (int)(y * k); };
    auto R2 = [&](double x, double y, double w, double h, double rad, Col col) {
        SDL_Rect r = {px(x), py(y), std::max(1, (int)(w * k)), std::max(1, (int)(h * k))};
        fillRound(r, std::max(0, (int)(rad * k)), col);
    };
    auto CI = [&](double x, double y, double rad, Col col) {
        fillCircle(px(x), py(y), std::max(1, (int)(rad * k)), col);
    };
    switch (kind) {
        case 0:  // смартфон
            R2(10, 1, 20, 38, 4, c);
            R2(12, 6, 16, 28, 2, scrC);
            R2(17, 2, 6, 2, 1, d);
            CI(20, 37, 1.6, d);
            break;
        case 1:  // наушники
            CI(12, 22, 8, c);
            CI(28, 22, 8, c);
            R2(9, 22, 6, 14, 3, c);
            R2(25, 22, 6, 14, 3, c);
            CI(12, 22, 3, d);
            CI(28, 22, 3, d);
            break;
        case 2:  // часы
            R2(13, 0, 14, 8, 2, d);
            R2(13, 32, 14, 8, 2, d);
            R2(7, 6, 26, 28, 8, c);
            R2(10, 9, 20, 22, 6, scrC);
            R2(33, 16, 3, 8, 1, d);
            break;
        case 3:  // планшет
            R2(7, 1, 26, 38, 4, c);
            R2(10, 5, 20, 30, 2, scrC);
            CI(20, 37, 1.5, d);
            break;
        case 19:  // калькулятор
            R2(6, 2, 28, 36, 3, c);
            R2(9, 6, 22, 8, 1, scrC);
            for (int r = 0; r < 3; ++r)
                for (int q = 0; q < 3; ++q) R2(9 + q * 8, 18 + r * 6, 5, 4, 0, d);
            break;
        case 20:  // радиоприёмник
            R2(3, 10, 34, 22, 4, c);
            CI(12, 21, 6, d);
            R2(22, 15, 12, 12, 2, scrC);
            break;
        case 4:  // компьютер
            R2(6, 4, 28, 22, 3, c);
            R2(9, 7, 22, 16, 1, scrC);
            R2(1, 28, 38, 6, 2, c);
            R2(16, 29, 8, 1, 0, d);
            break;
        case 10:  // размер: линейка
            R2(3, 16, 34, 6, 2, c);
            for (int i = 0; i < 5; ++i) R2(4 + i * 8, 8, 2, 8, 0, c);
            R2(10, 28, 20, 2, 0, d);
            break;
        case 11:  // цвет: палитра
            CI(20, 20, 17, c);
            CI(14, 15, 4, {200, 50, 60, 255});
            CI(26, 15, 4, {50, 160, 90, 255});
            CI(20, 26, 4, {50, 100, 210, 255});
            break;
        case 12:  // экран
            R2(3, 5, 34, 24, 3, c);
            R2(6, 8, 28, 18, 1, scrC);
            R2(17, 29, 6, 4, 0, c);
            R2(11, 33, 18, 2, 1, c);
            break;
        case 13:  // процессор
            for (int i = 0; i < 3; ++i) {
                double o = 11 + i * 6;
                R2(o, 3, 2, 6, 0, d);
                R2(o, 31, 2, 6, 0, d);
                R2(3, o, 6, 2, 0, d);
                R2(31, o, 6, 2, 0, d);
            }
            R2(9, 9, 22, 22, 2, c);
            R2(14, 14, 12, 12, 1, scrC);
            break;
        case 14:  // батарея
            R2(3, 11, 32, 18, 3, c);
            R2(35, 16, 3, 8, 1, c);
            R2(6, 14, 20, 12, 1, {60, 210, 120, 255});
            break;
        case 15:  // динамики
            R2(9, 3, 22, 34, 6, c);
            CI(20, 15, 6, d);
            CI(20, 15, 3, scrC);
            CI(20, 29, 3, d);
            break;
        case 16:  // упаковка
            R2(4, 8, 32, 26, 2, c);
            R2(4, 8, 32, 8, 2, shade(c, 1.25));
            R2(18, 8, 4, 26, 0, d);
            break;
        case 34:  // источник ЦП: чип с выводами
            for (int i = 0; i < 3; ++i) {
                double o = 11 + i * 6;
                R2(o, 3, 2, 6, 0, d);
                R2(o, 31, 2, 6, 0, d);
                R2(3, o, 6, 2, 0, d);
                R2(31, o, 6, 2, 0, d);
            }
            R2(9, 9, 22, 22, 2, c);
            R2(14, 14, 12, 12, 1, scrC);
            break;
        case 33:  // ОС: окно с заголовком
            R2(4, 6, 32, 28, 3, c);
            R2(4, 6, 32, 6, 2, shade(c, 1.3));
            R2(8, 16, 24, 3, 0, scrC);
            R2(8, 22, 16, 3, 0, scrC);
            break;
        case 30:  // количество камер: три линзы
            CI(10, 20, 6, c); CI(20, 20, 6, c); CI(30, 20, 6, c);
            CI(10, 20, 3, d); CI(20, 20, 3, d); CI(30, 20, 3, d);
            break;
        case 31:  // разрешение: линза с лучами
            CI(20, 20, 14, c);
            CI(20, 20, 8, d);
            CI(20, 20, 3, scrC);
            break;
        case 32:  // сенсор: квадрат-матрица
            R2(6, 6, 28, 28, 2, c);
            for (int r = 0; r < 3; ++r)
                for (int q = 0; q < 3; ++q) R2(10 + q * 8, 10 + r * 8, 5, 5, 0, scrC);
            break;
        case 17:  // цена
            R2(4, 10, 32, 20, 4, c);
            CI(12, 20, 3, scrC);
            R2(18, 16, 14, 2, 0, scrC);
            R2(18, 22, 10, 2, 0, scrC);
            break;
        case 18:  // название
            R2(4, 8, 32, 24, 3, c);
            R2(8, 13, 24, 3, 0, scrC);
            R2(8, 20, 18, 3, 0, scrC);
            R2(8, 27, 22, 3, 0, scrC);
            break;
        default: break;
    }
}

// Плитка с иконкой слева и подписью справа (для категорий)
bool iconTile(SDL_Rect r, int kind, const std::string& label, bool enabled, bool sel) {
    bool hit = widget(r, enabled);
    bool hov = hovered(r);
    Col bg = !enabled ? C_BTND : (sel ? C_BTNSEL : (hov ? C_BTNH : C_BTN));
    fillRect(r, bg);
    fillRect(r.x, r.y, r.w, 2, shade(bg, 1.35));
    drawRect(r, shade(bg, 0.7));
    drawIcon(kind, r.x + 24, r.y + r.h / 2, 30, enabled ? C_TEXT : C_DIM);
    drawText(fitText(label, 2, r.w - 48), r.x + 44, r.y + (r.h - 14) / 2, 2, enabled ? C_TEXT : C_DIM);
    drawFocus(r);
    if (hit) sfx(SFX_CLICK);
    return hit;
}

void scrDesign() {
    Draft& d = G.draft;

    // Категории: плитки с иконками
    for (int c = 0; c < NCAT; ++c) {
        bool un = catUnlocked(c);
        SDL_Rect tr = {16 + c * 148, 66, 144, 58};
        if (iconTile(tr, c, un ? CATS[c].name : "Закрыто", un, d.cat == c)) {
            if (d.cat != c) {
                d.cat = c;
                d.p[P_SIZE] = CATS[c].idealSize;
                d.price = CATS[c].defPrice;
                fixDraft(d);
            }
        }
    }

    // Список параметров: иконка, название, управление, сегменты
    const Cat& cat = CATS[d.cat];
    SDL_Rect view = {524, 138, 740, 574};
    beginArea(0, view);
    int sy = scr(0);
    int rows = NPARAM + 2;
    for (int i = 0; i < rows; ++i) {
        int ry = view.y + i * 72 - sy;
        SDL_Rect row = {view.x, ry, view.w - 8, 64};
        fillRect(row, C_PANEL);
        drawRect(row, C_BORDER);
        if (i < NPARAM) {
            int p = i;
            drawIcon(p < 7 ? 10 + p : 23 + p, row.x + 30, ry + 32, 40, C_ACC);
            drawText(PARAM_NAMES[p], row.x + 60, ry + 25, 2, C_TEXT);
            if (cat.w[p] <= 0) {
                drawText("Не используется в этой категории", row.x + 232, ry + 25, 2, C_DIM);
                continue;
            }
            int n = optCount(p);
            bool allowed[8];
            for (int k = 0; k < n; ++k) allowed[k] = tierUnlocked(p, k);
            if (button({row.x + 170, ry + 6, 52, 52}, "-", d.p[p] > 0)) d.p[p]--;
            SDL_Rect sl = {row.x + 232, ry + 6, 370, 52};
            slider(sl, n, d.p[p], allowed);
            fillRect(sl, C_PANEL2);
            drawTextC(fitText(opt(p, d.p[p]).name, 2, 360), sl.x + sl.w / 2, sl.y + 6, 2, C_TEXT);
            int segW = sl.w / n;
            for (int k = 0; k < n; ++k) {
                SDL_Rect sg = {sl.x + k * segW + 2, sl.y + 28, segW - 4, 18};
                Col c;
                if (p == P_COLOR) c = COLOR_RGB[k];
                else if (!allowed[k]) c = {44, 48, 62, 255};
                else c = k <= d.p[p] ? C_ACC : C_BTN;
                fillRect(sg, c);
                if (k == d.p[p]) drawRect({sg.x - 2, sg.y - 2, sg.w + 4, sg.h + 4}, C_TEXT, 2);
                if (p == P_COLOR && k == G.trend) drawText("*", sg.x + sg.w / 2 - 5, sg.y + 2, 2, C_YEL);
            }
            bool canUp = d.p[p] < n - 1 && allowed[d.p[p] + 1];
            if (button({row.x + 618, ry + 6, 52, 52}, "+", canUp)) d.p[p]++;
            std::string cs;
            if (p == P_SIZE) cs = "x" + fmt1(SIZE_OPT[d.p[p]].cost);
            else cs = "+$" + fmtNum(opt(p, d.p[p]).cost);
            drawTextR(cs, row.x + row.w - 6, ry + 46, 1, C_DIM);
        } else if (i == NPARAM) {
            drawIcon(17, row.x + 30, ry + 32, 40, C_ACC);
            drawText("Цена", row.x + 60, ry + 25, 2, C_TEXT);
            int st = priceStep(d.price);
            if (button({row.x + 170, ry + 6, 52, 52}, "-", d.price > 10)) d.price = std::max(10, d.price - st);
            double uc = draftUnitCost(d);
            double fair = fairPrice(uc, scoreFromQ(draftQuality(d)));
            fillRect(row.x + 232, ry + 6, 370, 52, C_PANEL2);
            drawTextC("$" + fmtNum(d.price), row.x + 417, ry + 10, 2, C_GREEN);
            drawTextC("рекомендуемая: $" + fmtNum(fair), row.x + 417, ry + 34, 2, C_DIM);
            if (button({row.x + 618, ry + 6, 52, 52}, "+", d.price < 99999)) d.price += st;
        } else {
            drawIcon(18, row.x + 30, ry + 32, 40, C_ACC);
            drawText("Название", row.x + 60, ry + 25, 2, C_TEXT);
            if (button({row.x + 170, ry + 6, 52, 52}, "<")) d.suffix = (d.suffix + 9) % 10;
            fillRect(row.x + 232, ry + 6, 370, 52, C_PANEL2);
            drawTextC(fitText(draftName(d), 2, 360), row.x + 417, ry + 24, 2, C_TEXT);
            if (button({row.x + 618, ry + 6, 52, 52}, ">")) d.suffix = (d.suffix + 1) % 10;
        }
    }
    endArea(rows * 72);

    // Предпросмотр слева: как будет выглядеть устройство
    panel(16, 138, 492, 574, "");
    fillRect(28, 150, 468, 236, C_PANEL2);
    fillCircle(262, 268, 104, {30, 46, 78, 255});
    drawDevice(262, 268, d.cat, d.p[P_SIZE], d.p[P_COLOR], d.p[P_SCREEN] + 1, d.p[P_CAMN], d.p[P_CAMSZ]);
    drawIcon(d.cat, 480, 166, 26, C_ACC);
    drawText(fitText(draftName(d), 2, 400), 40, 162, 2, C_DIM);

    double uc = draftUnitCost(d);
    double tool = toolingCost(d);
    double q = draftQuality(d);
    double sc = scoreFromQ(q);
    double days = draftDevTotal(d) / devSpeed();
    int y = 398;
    drawText("Себестоимость: " + fmtMoney(uc), 36, y, 2, C_TEXT); y += 26;
    double margin = d.price - uc;
    drawText("Маржа: " + fmtMoney(margin) + " (" + std::to_string((int)(margin / std::max(1, d.price) * 100)) + "%)",
             36, y, 2, margin >= 0 ? C_GREEN : C_RED);
    y += 26;
    drawText("Оснастка (разово): " + fmtMoney(tool), 36, y, 2, C_TEXT); y += 26;
    drawText("Разработка: ≈ " + std::to_string((int)std::ceil(days)) + " дн.", 36, y, 2, C_TEXT); y += 34;
    drawText("Ожидаемая оценка", 36, y + 8, 2, C_DIM);
    drawTextR(fmt1(sc), 496, y - 2, 4, ratingCol(sc));
    y += 36;
    for (int k = 0; k < 10; ++k)
        fillRect(36 + k * 46, y, 42, 12, (k + 1) <= (int)std::round(sc) ? ratingCol(sc) : C_PANEL2);
    y += 24;
    if (button({36, 586, 452, 46}, "Своя платформа: процессор и ОС")) {
        retScreen = SC_DESIGN;
        setScreen(SC_PLATFORM);
    }
    bool can = !G.inDev && G.money >= tool;
    std::string lbl = G.inDev ? "Уже идёт разработка" : (G.money < tool ? "Не хватает денег" : "Начать разработку");
    if (button({36, 650, 460, 54}, lbl, can, can)) {
        G.money -= tool;
        G.monExp += tool;
        G.dev = d;
        G.devName = draftName(d);
        G.gen[d.cat]++;
        G.inDev = true;
        G.devReady = false;
        G.devProg = 0;
        G.devTotal = draftDevTotal(d);
        logMsg("Начата разработка «" + G.devName + "»");
        toast("Разработка началась!");
        setScreen(SC_MAIN);
    }
}

void projectPanel(int x, const std::string& title, Proj& pr, const PO (&st)[3][3], const char* const* names,
                  const char* what) {
    panel(x, 96, 612, 600, "");
    drawText(title, x + 16, 108, 3, C_ACC);
    if (pr.st >= 3) {
        drawText("Готов. Качество: " + std::to_string(pr.q), x + 16, 160, 2, C_GREEN);
        return;
    }
    drawText("Этап " + std::to_string(pr.st + 1) + " из 3: " + names[pr.st], x + 16, 160, 2, C_TEXT);
    if (pr.pick >= 0) {
        const PO& o = st[pr.st][pr.pick];
        int left = (int)std::ceil(std::max(0.0, o.days - pr.prog));
        drawText("Идёт: " + std::string(o.n) + ", осталось " + std::to_string(left) + " дн.", x + 16, 200, 2, C_DIM);
        progressBar({x + 16, 232, 580, 22}, pr.prog / o.days, C_GREEN);
    } else {
        drawText("Выберите подход (оплата при старте этапа):", x + 16, 200, 2, C_DIM);
        for (int i = 0; i < 3; ++i) {
            const PO& o = st[pr.st][i];
            std::string lbl = std::string(o.n) + "  $" + fmtNum(o.cost) + "  " + std::to_string(o.days) + " дн.  +" +
                              std::to_string(o.q);
            SDL_Rect r = {x + 16, 270 + i * 74, 580, 60};
            if (button(r, lbl, G.money >= o.cost)) {
                G.money -= o.cost;
                G.monExp += o.cost;
                pr.pick = i;
                pr.prog = 0;
                logMsg(std::string(what) + ": начат этап «" + o.n + "»");
                toast("Этап начат");
            }
        }
    }
    drawText("Накоплено качества: " + std::to_string(pr.q), x + 16, 520, 2, C_TEXT);
}

void scrPlatform() {
    drawText("Своя платформа: этапы разработки. Качество складывается из выбранных подходов.", 24, 76, 2, C_ACC);
    projectPanel(16, "Процессор", PRJ_CPU, CPU_STAGES, CPU_NAMES, "Процессор");
    projectPanel(652, "Операционная система", PRJ_OS, OS_STAGES, OS_NAMES, "ОС");
}

void salesChart(SDL_Rect r, const Product& p) {
    fillRect(r, C_PANEL2);
    drawRect(r, C_BORDER);
    std::vector<int> w = p.weekly;
    if (p.onSale) w.push_back((int)p.weekAcc);
    int n = (int)w.size();
    int first = std::max(0, n - 26);
    int mx = 1;
    for (int i = first; i < n; ++i) mx = std::max(mx, w[(size_t)i]);
    drawText("Продажи по неделям (шт.)", r.x + 10, r.y + 8, 2, C_DIM);
    drawTextR("макс " + fmtNum(mx), r.x + r.w - 10, r.y + 8, 2, C_DIM);
    int bw = std::max(4, (r.w - 20) / 26);
    int base = r.y + r.h - 12, top = r.y + 34;
    for (int i = first; i < n; ++i) {
        int h = (int)((double)w[(size_t)i] / mx * (base - top));
        bool cur = p.onSale && i == n - 1;
        fillRect(r.x + 10 + (i - first) * bw, base - h, bw - 3, h, cur ? C_DIM : C_ACC);
    }
    if (n == 0) drawTextC("Нет данных", r.x + r.w / 2, r.y + r.h / 2, 2, C_DIM);
}

void scrLaunch() {
    // Результаты запуска
    if (resIdx >= 0 && resIdx < (int)G.products.size()) {
        const Product& p = G.products[(size_t)resIdx];
        panel(140, 84, 1000, 620, "");
        drawTextC("Запуск: " + p.name, W / 2, 104, 3, C_ACC);
        drawDevice(330, 300, p.cat, p.p[P_SIZE], p.p[P_COLOR], p.p[P_SCREEN] + 1);
        double shown = std::min(p.rating, p.rating * resT / 1.5);
        drawText("Средняя оценка", 560, 170, 2, C_DIM);
        drawText(fmt1(shown), 560, 196, 8, ratingCol(shown));
        drawText("/10", 560 + textW(fmt1(shown), 8) + 12, 230, 3, C_DIM);
        if (resT > 1.5) {
            for (int i = 0; i < 4; ++i) {
                if (resT < 1.5 + i * 0.3) break;
                drawText(OUTLETS[i], 560, 280 + i * 32, 2, C_TEXT);
                drawText(fmt1(resOutlet[i]), 800, 280 + i * 32, 2, ratingCol(resOutlet[i]));
            }
            const char* rv;
            double r = p.rating;
            if (r >= 9) rv = "Шедевр! Лучшее устройство года.";
            else if (r >= 7.5) rv = "Отличный девайс, рекомендуем.";
            else if (r >= 6) rv = "Хорошо, но без сюрпризов.";
            else if (r >= 4.5) rv = "Средне. Есть варианты получше.";
            else if (r >= 3) rv = "Слабо. Слишком много компромиссов.";
            else rv = "Провал. Обходите стороной.";
            drawText(std::string("«") + rv + "»", 200, 470, 2, C_TEXT);
            drawText("Репутация: " + std::string(resRepDelta >= 0 ? "+" : "") + fmt1(resRepDelta), 200, 510, 2,
                     resRepDelta >= 0 ? C_GREEN : C_RED);
            drawText("Цена: " + fmtMoney(p.price) + "   Себестоимость: " + fmtMoney(p.unitCost), 200, 540, 2, C_DIM);
        }
        if (button({440, 620, 400, 60}, "Отлично!", true, true)) resIdx = -1;
        return;
    }
    // Подготовка к запуску
    if (G.devReady) {
        panel(16, 76, 1248, 636, "");
        drawText("Готов к запуску: " + G.devName, 40, 96, 3, C_ACC);
        fillRect(40, 140, 420, 300, C_PANEL2);
        drawDevice(250, 290, G.dev.cat, G.dev.p[P_SIZE], G.dev.p[P_COLOR], G.dev.p[P_SCREEN] + 1);
        double uc = draftUnitCost(G.dev);
        double sc = scoreFromQ(draftQuality(G.dev));
        drawText("Категория: " + std::string(CATS[G.dev.cat].name), 40, 460, 2, C_TEXT);
        drawText("Цена: " + fmtMoney(G.dev.price) + "  (себест. " + fmtMoney(uc) + ")", 40, 488, 2, C_TEXT);
        drawText("Ожидаемая оценка: " + fmt1(sc), 40, 516, 2, ratingCol(sc));
        int st = priceStep(G.dev.price);
        if (button({40, 552, 200, 54}, "- цена", G.dev.price > 10)) G.dev.price = std::max(10, G.dev.price - st);
        if (button({256, 552, 200, 54}, "+ цена")) G.dev.price += st;
        drawText("Презентация", 500, 140, 2, C_DIM);
        double k = 0.7 + 0.3 * marketSize();
        for (int i = 0; i < 4; ++i) {
            SDL_Rect r = {500, 170 + i * 80, 740, 68};
            double c = PRES[i].cost * k;
            bool can = G.money >= c;
            if (button(r, "", can, presChoice == i)) presChoice = i;
            drawText(PRES[i].name, r.x + 16, r.y + 12, 2, can ? C_TEXT : C_DIM);
            drawText(i == 0 ? std::string("Бесплатно") : fmtMoney(c), r.x + 16, r.y + 40, 2, C_DIM);
            if (i > 0) drawTextR("+" + fmt1(PRES[i].bonus) + " к оценке", r.x + r.w - 16, r.y + 26, 2, C_GREEN);
        }
        if (presChoice > 0 && G.money < PRES[presChoice].cost * k) presChoice = 0;
        if (button({500, 600, 740, 80}, "Запустить продажи!", true, true)) launchProduct();
        return;
    }
    // Список продуктов
    if (G.products.empty()) {
        panel(16, 76, 1248, 636, "");
        drawTextC("Пока нет выпущенных устройств.", W / 2, 300, 3, C_TEXT);
        drawTextC(G.inDev ? "Дождитесь окончания разработки." : "Создайте первое устройство в конструкторе.", W / 2,
                  350, 2, C_DIM);
        if (!G.inDev && button({440, 420, 400, 64}, "В конструктор", true, true)) setScreen(SC_DESIGN);
        return;
    }
    if (selProduct < 0 || selProduct >= (int)G.products.size()) selProduct = (int)G.products.size() - 1;
    drawText("Устройства (" + std::to_string(G.products.size()) + ")", 24, 84, 2, C_ACC);
    SDL_Rect view = {16, 110, 420, 602};
    beginArea(1, view);
    int sy = scr(1);
    int n = (int)G.products.size();
    for (int j = 0; j < n; ++j) {
        int i = n - 1 - j;
        const Product& p = G.products[(size_t)i];
        SDL_Rect r = {view.x, view.y + j * 70 - sy, view.w - 10, 62};
        if (widget(r, true)) { selProduct = i; sfx(SFX_CLICK); }
        fillRect(r, i == selProduct ? C_BTNSEL : (hovered(r) ? C_BTNH : C_PANEL));
        drawRect(r, C_BORDER);
        drawText(fitText(p.name, 2, 300), r.x + 10, r.y + 10, 2, C_TEXT);
        drawText(p.onSale ? "В продаже" : "Снят", r.x + 10, r.y + 36, 2, p.onSale ? C_GREEN : C_DIM);
        drawTextR(fmt1(p.rating), r.x + r.w - 10, r.y + 20, 3, ratingCol(p.rating));
        drawFocus(r);
    }
    endArea(n * 70);

    Product& p = G.products[(size_t)selProduct];
    panel(452, 76, 812, 636, "");
    drawText(p.name, 472, 92, 3, C_ACC);
    drawText(std::string(CATS[p.cat].name) + " / " + PRES[p.pres].name, 472, 124, 2, C_DIM);
    drawTextR(fmt1(p.rating), 1244, 88, 5, ratingCol(p.rating));
    int y = 156;
    drawText("Цена: " + fmtMoney(p.price), 472, y + 18, 2, C_TEXT);
    if (p.onSale) {
        int st = priceStep(p.price);
        if (button({760, y, 120, 52}, "- $" + std::to_string(st), p.price > 10)) p.price = std::max(10, p.price - st);
        if (button({890, y, 120, 52}, "+ $" + std::to_string(st))) p.price += st;
    }
    drawText("реком. " + fmtMoney(fairPrice(p.unitCost, p.rating)), 1030, y + 18, 2, C_DIM);
    y += 66;
    drawText("Себестоимость: " + fmtMoney(p.unitCost), 472, y, 2, C_TEXT);
    drawText("Продажи вчера: " + fmtNum(p.lastDay) + " шт.", 860, y, 2, C_TEXT);
    y += 28;
    drawText("Продано всего: " + fmtNum(p.units) + " шт.", 472, y, 2, C_TEXT);
    drawText("Выручка: " + fmtMoney(p.revenue), 860, y, 2, C_TEXT);
    y += 28;
    drawText("Прибыль: " + fmtMoney(p.profit), 472, y, 2, p.profit >= 0 ? C_GREEN : C_RED);
    drawText("В продаже: " + std::to_string(G.day - p.launchDay) + " дн.", 860, y, 2, C_TEXT);
    salesChart({468, 310, 780, 300}, p);
    if (p.onSale) {
        if (button({468, 630, 380, 60}, "Снять с продаж")) {
            p.onSale = false;
            logMsg("«" + p.name + "» снят с продаж.");
        }
    } else {
        drawText("Продажи завершены", 472, 650, 2, C_DIM);
    }
    if (button({868, 630, 380, 60}, "Новое устройство", !G.inDev)) setScreen(SC_DESIGN);
}

void scrOffice() {
    panel(16, 74, 600, 250, "Офис");
    drawText(OFFICE_NAME[G.office], 32, 106, 3, C_TEXT);
    drawText("Сотрудники: " + std::to_string(G.staff.size()) + "/" + std::to_string(OFFICE_CAP[G.office]), 32, 146, 2,
             C_TEXT);
    drawText("Бонус скорости: +" + std::to_string((int)std::round((OFFICE_MULT[G.office] - 1) * 100)) + "%", 32, 172,
             2, C_TEXT);
    drawText("Аренда: " + fmtMoney(OFFICE_RENT[G.office]) + "/мес", 32, 198, 2, C_TEXT);
    // Здание
    int bh = 40 + G.office * 22, bx = 470, by = 230 - bh;
    fillRect(bx, by, 120, bh, C_BTN);
    for (int fl = 0; fl < bh / 22; ++fl)
        for (int wx = 0; wx < 4; ++wx) fillRect(bx + 10 + wx * 27, by + 8 + fl * 22, 18, 12, C_YEL);
    if (G.office < NOFFICE - 1) {
        double c = OFFICE_COST[G.office + 1];
        if (button({32, 254, 568, 56}, "Переезд: " + std::string(OFFICE_NAME[G.office + 1]) + " за " + fmtMoney(c),
                   G.money >= c)) {
            G.money -= c;
            G.monExp += c;
            G.office++;
            logMsg(std::string("Компания переехала: ") + OFFICE_NAME[G.office]);
            sfx(SFX_DONE);
        }
    } else {
        drawText("Максимальный уровень офиса", 32, 274, 2, C_GREEN);
    }
    panel(16, 334, 600, 120, "Показатели");
    drawText("Очки исследований: " + fmt1(researchRate()) + "/день", 32, 366, 2, C_TEXT);
    drawText("Скорость разработки: x" + fmt1(devSpeed()), 32, 392, 2, C_TEXT);
    drawText("Зарплаты: " + fmtMoney(salaries()) + "/мес", 32, 418, 2, C_TEXT);

    panel(16, 464, 600, 248, "Кандидаты");
    if (button({400, 470, 200, 48}, "Новые ($800)", G.money >= 800)) {
        G.money -= 800;
        G.monExp += 800;
        refreshCandidates();
    }
    bool full = (int)G.staff.size() >= OFFICE_CAP[G.office];
    for (size_t i = 0; i < G.cand.size(); ++i) {
        const Staff& s = G.cand[i];
        int y = 526 + (int)i * 62;
        drawText(s.name + ", " + ROLE_NAME[s.role], 32, y + 4, 2, C_TEXT);
        drawText("Навык " + std::to_string(s.skill) + "/10   " + fmtMoney(s.salary) + "/мес", 32, y + 28, 2, C_DIM);
        if (button({430, y, 170, 52}, full ? "Нет мест" : "Нанять", !full && G.money >= s.salary)) {
            G.money -= s.salary;
            G.monExp += s.salary;
            G.staff.push_back(s);
            logMsg("Нанят: " + s.name + " (" + ROLE_NAME[s.role] + ")");
            G.cand.erase(G.cand.begin() + (long)i);
            break;
        }
    }
    if (G.cand.empty()) drawText("Кандидатов нет. Ждите начала месяца.", 32, 560, 2, C_DIM);

    drawText("Штат (" + std::to_string(G.staff.size()) + ")", 640, 84, 2, C_ACC);
    SDL_Rect view = {632, 110, 632, 602};
    beginArea(2, view);
    int sy = scr(2);
    if (G.staff.empty()) drawText("Пока вы работаете в одиночку.", 648, 130 - sy, 2, C_DIM);
    for (size_t i = 0; i < G.staff.size(); ++i) {
        const Staff& s = G.staff[i];
        SDL_Rect r = {view.x, view.y + (int)i * 66 - sy, view.w - 10, 60};
        fillRect(r, C_PANEL);
        drawRect(r, C_BORDER);
        drawText(s.name, r.x + 12, r.y + 8, 2, C_TEXT);
        drawText(ROLE_NAME[s.role], r.x + 12, r.y + 34, 2, s.role == 0 ? C_ORANGE : (s.role == 1 ? C_ACC : C_GREEN));
        for (int k = 0; k < 10; ++k) fillRect(r.x + 220 + k * 12, r.y + 12, 9, 12, k < s.skill ? C_YEL : C_PANEL2);
        drawText(fmtMoney(s.salary) + "/мес", r.x + 220, r.y + 34, 2, C_DIM);
        if (button({r.x + r.w - 150, r.y + 6, 140, 48}, "Уволить")) {
            logMsg("Уволен: " + s.name);
            G.staff.erase(G.staff.begin() + (long)i);
            break;
        }
    }
    endArea((int)G.staff.size() * 66);
}

void scrResearch() {
    panel(16, 74, 1248, 82, "");
    double rate = researchRate();
    drawText("Очки исследований: " + fmt1(rate) + " в день", 32, 86, 2, C_ACC);
    if (G.researching >= 0) {
        const Tech& t = TECHS[(size_t)G.researching];
        drawText(fitText("Изучается: " + t.name, 2, 700), 32, 116, 2, C_TEXT);
        progressBar({760, 112, 480, 22}, G.researchProg / t.cost, C_GREEN);
    } else {
        drawText("Выберите технологию для изучения", 32, 116, 2, C_DIM);
    }
    SDL_Rect view = {16, 166, 1248, 546};
    beginArea(3, view);
    int sy = scr(3);
    for (size_t i = 0; i < TECHS.size(); ++i) {
        const Tech& t = TECHS[i];
        SDL_Rect r = {view.x, view.y + (int)i * 66 - sy, view.w - 10, 60};
        bool done = techDone((int)i);
        bool rel = calYear() >= t.year;
        bool avail = !done && rel && (t.prereq < 0 || techDone(t.prereq));
        bool cur = G.researching == (int)i;
        fillRect(r, cur ? shade(C_BTNSEL, 0.6) : C_PANEL);
        drawRect(r, C_BORDER);
        drawText(t.name, r.x + 12, r.y + 8, 2, done ? C_GREEN : (avail ? C_TEXT : C_DIM));
        const char* desc = t.kind == 0 ? "Новый вариант компонента"
                         : t.kind == 1 ? "Открывает новую категорию устройств"
                         : t.kind == 2 ? "-8% к себестоимости производства"
                         : t.kind == 3 ? "+15% к скорости разработки"
                                       : "+8% к скорости разработки (нейросети)";
        std::string descS = desc;
        if (!rel) descS = "Появится в " + std::to_string(t.year) + " г.";
        drawText(descS, r.x + 12, r.y + 34, 2, C_DIM);
        if (!done) {
            double left = std::max(0.0, t.cost - (cur ? G.researchProg : 0.0));
            drawText(fmtNum(t.cost) + " очк.", r.x + 600, r.y + 8, 2, C_TEXT);
            drawText("≈ " + std::to_string((int)std::ceil(left / rate)) + " дн.", r.x + 600, r.y + 34, 2, C_DIM);
        }
        std::string lbl;
        bool en;
        if (done) { lbl = "Изучено"; en = false; }
        else if (cur) { lbl = "Отменить"; en = true; }
        else if (!rel) { lbl = "Скоро"; en = false; }
        else if (!avail) { lbl = "Нужна пред."; en = false; }
        else if (G.researching >= 0) { lbl = "Занято"; en = false; }
        else { lbl = "Изучить"; en = true; }
        if (cur) progressBar({r.x + 800, r.y + 22, 200, 16}, G.researchProg / t.cost, C_GREEN);
        if (button({r.x + r.w - 220, r.y + 6, 210, 48}, lbl, en, !done && en && !cur)) {
            if (cur) { G.researching = -1; G.researchProg = 0; }
            else { G.researching = (int)i; G.researchProg = 0; }
        }
    }
    endArea((int)TECHS.size() * 66);
}

void scrMarket() {
    drawText("Рынки: объём x" + fmt1(marketSize()), 24, 84, 2, C_ACC);
    SDL_Rect view = {16, 110, 700, 602};
    beginArea(4, view);
    int sy = scr(4);
    for (int i = 0; i < NMARKET; ++i) {
        const Market& m = MARKETS[i];
        SDL_Rect r = {view.x, view.y + i * 66 - sy, view.w - 10, 60};
        fillRect(r, G.market[i] ? shade(C_GREEN, 0.3) : C_PANEL);
        drawRect(r, C_BORDER);
        drawText(m.name, r.x + 12, r.y + 8, 2, C_TEXT);
        drawText("Объём x" + fmt1(m.size) + "   Реп. ≥ " + std::to_string((int)m.rep), r.x + 12, r.y + 34, 2, C_DIM);
        if (G.market[i]) {
            drawTextR("Открыт", r.x + r.w - 20, r.y + 22, 2, C_GREEN);
        } else {
            bool repOk = G.rep >= m.rep;
            bool can = repOk && G.money >= m.cost;
            if (button({r.x + r.w - 260, r.y + 6, 250, 48}, repOk ? "Открыть " + fmtShort(m.cost) : "Мало репутации",
                       can)) {
                G.money -= m.cost;
                G.monExp += m.cost;
                G.market[i] = true;
                logMsg(std::string("Открыт рынок: ") + m.name);
                sfx(SFX_CASH);
            }
        }
    }
    endArea(NMARKET * 66);

    panel(732, 74, 532, 638, "Маркетинг");
    drawText("Ажиотаж", 748, 110, 2, C_TEXT);
    progressBar({860, 108, 300, 18}, G.hype / 4.0, C_ORANGE);
    drawText("Спрос: x" + fmt1(1.0 + 0.5 * G.hype), 748, 138, 2, C_DIM);
    double k = std::sqrt(marketSize());
    for (int i = 0; i < 4; ++i) {
        SDL_Rect r = {748, 172 + i * 80, 500, 68};
        double c = CAMPS[i].cost * k;
        bool can = G.money >= c && G.hype < 3.9;
        if (button(r, "", can)) {
            G.money -= c;
            G.monExp += c;
            G.hype = std::min(4.0, G.hype + CAMPS[i].hype);
            logMsg(std::string("Кампания: ") + CAMPS[i].name);
            sfx(SFX_CASH);
        }
        drawText(CAMPS[i].name, r.x + 16, r.y + 12, 2, can ? C_TEXT : C_DIM);
        drawText(fmtMoney(c), r.x + 16, r.y + 40, 2, C_DIM);
        drawTextR("+" + fmt1(CAMPS[i].hype) + " ажиотажа", r.x + r.w - 16, r.y + 26, 2, C_GREEN);
    }
    drawText("Модный цвет года:", 748, 506, 2, C_TEXT);
    fillRect(980, 500, 40, 26, COLOR_RGB[G.trend]);
    drawText(COLOR_OPT[G.trend].name, 1030, 506, 2, C_YEL);
    drawText("Ажиотаж затухает ~2% в день.", 748, 548, 2, C_DIM);
    drawText("Новые рынки увеличивают продажи", 748, 574, 2, C_DIM);
    drawText("всех устройств в продаже.", 748, 600, 2, C_DIM);
    drawText("Фанаты: " + fmtNum(G.fans), 748, 640, 2, C_TEXT);
}

void financeCharts() {
    std::vector<double> inc, exp;
    int n = (int)G.histInc.size();
    int first = std::max(0, n - 11);
    for (int i = first; i < n; ++i) { inc.push_back(G.histInc[(size_t)i]); exp.push_back(G.histExp[(size_t)i]); }
    inc.push_back(G.monInc);
    exp.push_back(G.monExp);
    int m = (int)inc.size();
    int curMonth = G.day / 30;
    double mx = 1000;
    for (int i = 0; i < m; ++i) mx = std::max(mx, std::max(inc[(size_t)i], exp[(size_t)i]));
    SDL_Rect r = {16, 134, 1248, 330};
    fillRect(r, C_PANEL);
    drawRect(r, C_BORDER);
    drawText("Доходы и расходы по месяцам", 32, 144, 2, C_ACC);
    fillRect(820, 146, 14, 14, C_GREEN); drawText("Доходы", 842, 144, 2, C_TEXT);
    fillRect(960, 146, 14, 14, C_RED); drawText("Расходы", 982, 144, 2, C_TEXT);
    int left = r.x + 90, base = r.y + r.h - 34, top = r.y + 44;
    for (int g = 0; g <= 4; ++g) {
        int yy = base - (base - top) * g / 4;
        fillRect(left, yy, r.w - 110, 1, C_BORDER);
        drawTextR(fmtShort(mx * g / 4), left - 8, yy - 6, 2, C_DIM);
    }
    int gw = (r.w - 110) / 12;
    for (int i = 0; i < m; ++i) {
        int gx = left + i * gw + 8;
        int bw = (gw - 20) / 2;
        int hi = (int)(inc[(size_t)i] / mx * (base - top));
        int he = (int)(exp[(size_t)i] / mx * (base - top));
        bool cur = i == m - 1;
        fillRect(gx, base - hi, bw, hi, cur ? shade(C_GREEN, 0.6) : C_GREEN);
        fillRect(gx + bw + 2, base - he, bw, he, cur ? shade(C_RED, 0.6) : C_RED);
        int mo = curMonth - (m - 1 - i);
        drawText(MONTHS_S[((mo % 12) + 12) % 12], gx, base + 10, 2, cur ? C_YEL : C_DIM);
    }
    // Линия прибыли
    SDL_Rect r2 = {16, 474, 1248, 160};
    fillRect(r2, C_PANEL);
    drawRect(r2, C_BORDER);
    drawText("Прибыль", 32, 484, 2, C_ACC);
    double pm = 1000;
    for (int i = 0; i < m; ++i) pm = std::max(pm, std::fabs(inc[(size_t)i] - exp[(size_t)i]));
    int zy = r2.y + r2.h / 2 + 8, amp = r2.h / 2 - 22;
    fillRect(left, zy, r2.w - 110, 1, C_DIM);
    drawTextR("0", left - 8, zy - 6, 2, C_DIM);
    drawTextR(fmtShort(pm), left - 8, zy - amp - 6, 2, C_DIM);
    int px = -1, py = -1;
    for (int i = 0; i < m; ++i) {
        double pr = inc[(size_t)i] - exp[(size_t)i];
        int x = left + i * gw + gw / 2;
        int y = zy - (int)(pr / pm * amp);
        if (px >= 0) {
            setc(C_YEL);
            SDL_RenderDrawLine(R, px, py, x, y);
            SDL_RenderDrawLine(R, px, py + 1, x, y + 1);
        }
        fillCircle(x, y, 4, pr >= 0 ? C_GREEN : C_RED);
        px = x; py = y;
    }
    double tInc = G.monInc, tExp = G.monExp;
    for (int i = first; i < n; ++i) { tInc += G.histInc[(size_t)i]; tExp += G.histExp[(size_t)i]; }
    drawText("За 12 мес: доходы " + fmtMoney(tInc) + ",  расходы " + fmtMoney(tExp) + ",  итог " +
                 fmtMoney(tInc - tExp), 24, 650, 2, tInc >= tExp ? C_GREEN : C_RED);
    drawText("Зарплаты " + fmtMoney(salaries()) + "/мес,  аренда " + fmtMoney(OFFICE_RENT[G.office]) + "/мес", 24,
             680, 2, C_DIM);
}

void scrStats() {
    const char* tabs[3] = {"Финансы", "Рейтинги рынка", "Устройства"};
    for (int i = 0; i < 3; ++i)
        if (button({16 + i * 250, 72, 240, 52}, tabs[i], true, statsTab == i) && statsTab != i) {
            statsTab = i;
            U.scroll[5] = 0;
        }
    if (statsTab == 0) { financeCharts(); return; }
    if (statsTab == 1) {
        panel(16, 134, 1248, 578, "Доля рынка (по выручке за месяц)");
        double last = G.histInc.empty() ? 0 : G.histInc.back();
        double cur = G.monInc * 30.0 / ((G.day % 30) + 1);
        double pl = std::max(last, cur) / 1000.0;
        struct Row { std::string n; double s; bool me; };
        std::vector<Row> rows;
        rows.push_back({G.company, pl, true});
        for (int i = 0; i < NCOMP; ++i) rows.push_back({COMP_NAMES[i], G.comp[i], false});
        double tot = 0;
        for (size_t i = 0; i < rows.size(); ++i) tot += rows[i].s;
        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.s > b.s; });
        for (size_t i = 0; i < rows.size(); ++i) {
            int y = 180 + (int)i * 70;
            double sh = tot > 0 ? rows[i].s / tot : 0;
            drawText(std::to_string(i + 1) + ". " + fitText(rows[i].n, 2, 300), 40, y + 8, 2,
                     rows[i].me ? C_YEL : C_TEXT);
            progressBar({380, y, 700, 30}, sh / std::max(0.01, rows[0].s / tot), rows[i].me ? C_YEL : C_ACC);
            drawText(fmt1(sh * 100) + "%", 1100, y + 8, 2, C_TEXT);
        }
        drawText("Ожидания рынка растут каждый год: используйте новые технологии.", 40, 610, 2, C_DIM);
        drawText("Планка качества сейчас: " + fmt1(expectation()), 40, 640, 2, C_DIM);
        return;
    }
    // Устройства по рейтингу
    std::vector<int> idx;
    for (size_t i = 0; i < G.products.size(); ++i) idx.push_back((int)i);
    std::sort(idx.begin(), idx.end(),
              [](int a, int b) { return G.products[(size_t)a].rating > G.products[(size_t)b].rating; });
    SDL_Rect view = {16, 134, 1248, 578};
    beginArea(5, view);
    int sy = scr(5);
    if (idx.empty()) drawText("Выпущенных устройств пока нет.", 32, 160 - sy, 2, C_DIM);
    for (size_t k = 0; k < idx.size(); ++k) {
        const Product& p = G.products[(size_t)idx[k]];
        SDL_Rect r = {view.x, view.y + (int)k * 64 - sy, view.w - 10, 58};
        if (widget(r, true)) { selProduct = idx[k]; setScreen(SC_LAUNCH); sfx(SFX_CLICK); }
        fillRect(r, hovered(r) ? C_BTNH : C_PANEL);
        drawRect(r, C_BORDER);
        drawText(std::to_string(k + 1) + ".", r.x + 12, r.y + 20, 2, C_DIM);
        drawText(fitText(p.name, 2, 300), r.x + 60, r.y + 8, 2, C_TEXT);
        drawText(CATS[p.cat].name, r.x + 60, r.y + 32, 2, C_DIM);
        drawText(fmt1(p.rating), r.x + 400, r.y + 14, 3, ratingCol(p.rating));
        drawText("Продано " + fmtNum(p.units) + " шт.", r.x + 520, r.y + 8, 2, C_TEXT);
        drawText("Прибыль " + fmtMoney(p.profit), r.x + 520, r.y + 32, 2, p.profit >= 0 ? C_GREEN : C_RED);
        drawText(p.onSale ? "В продаже" : "Архив", r.x + 1000, r.y + 20, 2, p.onSale ? C_GREEN : C_DIM);
        drawFocus(r);
    }
    endArea((int)idx.size() * 64);
}

void scrPause() {
    fillRect(0, 0, W, H, {0, 0, 0, 150});
    panel(340, 70, 600, 600, "");
    drawTextC("Пауза", W / 2, 92, 4, C_ACC);
    drawText("Громкость", 380, 172, 2, C_TEXT);
    if (button({540, 156, 56, 52}, "-", A.vol > 0.001f)) A.vol = std::max(0.0f, A.vol - 0.1f);
    progressBar({606, 172, 200, 20}, A.vol, C_ACC);
    if (button({816, 156, 56, 52}, "+", A.vol < 0.999f)) { A.vol = std::min(1.0f, A.vol + 0.1f); }
    if (!A.dev) drawTextC("(звук недоступен)", W / 2, 216, 2, C_DIM);
    int y = 240;
    if (button({440, y, 400, 60}, "Продолжить", true, true)) setScreen(retScreen == SC_PAUSE ? SC_MAIN : retScreen);
    y += 72;
    if (button({440, y, 400, 60}, "Сохранить", !savePath.empty()))
        toast(saveGame() ? "Игра сохранена" : "Не удалось сохранить");
    y += 72;
    if (button({440, y, 400, 60}, "Загрузить", saveExists)) {
        if (loadGame()) { toast("Игра загружена"); setScreen(SC_MAIN); }
        else toast("Не удалось загрузить");
    }
    y += 72;
    if (button({440, y, 400, 60}, "Новая игра")) { newName = ""; setScreen(SC_NEW); }
    y += 72;
    if (button({440, y, 400, 60}, "Выход")) running = false;
    if (savePath.empty()) drawTextC("Сохранение недоступно на этом устройстве", W / 2, 640, 2, C_RED);
    else drawTextC("A - выбор, B - назад", W / 2, 640, 2, C_DIM);
}

void scrNew() {
    fillRect(0, 0, W, H, C_BG);
    for (int i = 0; i < 24; ++i) fillRect((i * 97 + (int)(uiTime * 20)) % W, (i * 53) % H, 3, 3, C_BORDER);
    drawTextC("ДЕВАЙС-КОРП", W / 2, 40, 7, C_ACC);
    drawTextC("Симулятор компании по разработке электроники", W / 2, 110, 2, C_DIM);
    drawText("Название компании", 340, 168, 2, C_TEXT);
    SDL_Rect f = {340, 192, 600, 60};
    if (widget(f, true)) {
        textActive = true;
        SDL_StartTextInput();
        sfx(SFX_CLICK);
    }
    fillRect(f, C_PANEL2);
    drawRect(f, textActive ? C_ACC : C_BORDER, 2);
    std::string shown = newName.empty() && !textActive ? std::string("Нажмите, чтобы ввести") : newName;
    drawText(fitText(shown, 3, 570), 356, 211, 3, newName.empty() ? C_DIM : C_TEXT);
    if (textActive && ((int)(uiTime * 2) % 2 == 0)) fillRect(356 + textW(newName, 3) + 4, 208, 3, 28, C_ACC);
    drawFocus(f);
    if (button({340, 264, 290, 54}, "Случайное имя")) newName = COMPANY_NAMES[irand(0, 9)];
    if (button({650, 264, 290, 54}, "Стереть", !newName.empty())) newName.clear();
    drawText("Стартовый капитал", 340, 338, 2, C_TEXT);
    const char* dn[3] = {"Лёгкий", "Обычный", "Хардкор"};
    const char* dm[3] = {"$500 000", "$150 000", "$50 000"};
    for (int i = 0; i < 3; ++i) {
        SDL_Rect r = {340 + i * 205, 364, 190, 80};
        if (button(r, "", true, capChoice == i)) capChoice = i;
        drawTextC(dn[i], r.x + r.w / 2, r.y + 16, 2, C_TEXT);
        drawTextC(dm[i], r.x + r.w / 2, r.y + 46, 2, C_GREEN);
    }
    if (button({340, 470, 600, 66}, "Основать компанию", true, true)) {
        if (textActive) { textActive = false; SDL_StopTextInput(); }
        newGame(newName, capChoice);
        setScreen(SC_DESIGN);  // сначала создаём первое устройство
        toast("Создайте первое устройство");
        sfx(SFX_DONE);
    }
    if (saveExists && button({340, 552, 600, 56}, "Продолжить сохранённую игру")) {
        if (textActive) { textActive = false; SDL_StopTextInput(); }
        if (loadGame()) setScreen(SC_MAIN);
        else toast("Сохранение повреждено");
    }
    drawTextC("Тач: тап и перетаскивание.  Геймпад: крестовина, A - выбор, B - назад, стики - прокрутка.", W / 2,
              650, 2, C_DIM);
    drawTextC("LB/RB - скорость времени, Start - меню.", W / 2, 676, 2, C_DIM);
}

void scrOver() {
    fillRect(0, 0, W, H, C_BG);
    drawTextC("БАНКРОТСТВО", W / 2, 120, 6, C_RED);
    drawTextC("Компания «" + G.company + "» не смогла расплатиться с долгами.", W / 2, 230, 2, C_TEXT);
    drawTextC("Прожито дней: " + std::to_string(G.day) + ",  устройств: " + std::to_string(G.products.size()) +
                  ",  фанатов: " + fmtNum(G.fans), W / 2, 270, 2, C_DIM);
    double best = 0;
    for (size_t i = 0; i < G.products.size(); ++i) best = std::max(best, G.products[i].rating);
    drawTextC("Лучшая оценка: " + fmt1(best), W / 2, 300, 2, C_DIM);
    if (button({440, 400, 400, 66}, "Новая игра", true, true)) { newName = ""; setScreen(SC_NEW); }
    if (button({440, 486, 400, 60}, "Выход")) running = false;
}

void renderFrame() {
    setc(C_BG);
    SDL_RenderClear(R);
    if (screen == SC_NEW) scrNew();
    else if (screen == SC_OVER) scrOver();
    else {
        Screen base = screen == SC_PAUSE ? retScreen : screen;
        if (screen != SC_PAUSE) {
            topBar();
            switch (base) {
                case SC_MAIN: scrMain(); break;
                case SC_DESIGN: scrDesign(); break;
                case SC_PLATFORM: scrPlatform(); break;
                case SC_LAUNCH: scrLaunch(); break;
                case SC_OFFICE: scrOffice(); break;
                case SC_RESEARCH: scrResearch(); break;
                case SC_MARKET: scrMarket(); break;
                case SC_STATS: scrStats(); break;
                default: break;
            }
        } else {
            // фон под паузой: без интерактивных элементов
            fillRect(0, 0, W, 64, C_PANEL2);
            drawText(fmtMoney(G.money), 340, 18, 3, G.money < 0 ? C_RED : C_GREEN);
            scrPause();
        }
    }
    if (toastT > 0) {
        int w = textW(toastMsg, 2) + 40;
        SDL_Rect r = {W / 2 - w / 2, H - 70, w, 44};
        fillRect(r, {10, 12, 20, 230});
        drawRect(r, C_ACC);
        drawTextC(toastMsg, W / 2, H - 55, 2, C_TEXT);
    }
    SDL_RenderPresent(R);
}

}  // namespace

// ============================================================================
//  main
// ============================================================================
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    bool haveAudio = SDL_InitSubSystem(SDL_INIT_AUDIO) == 0;
    bool havePad = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;

    SDL_Window* win = SDL_CreateWindow("Девайс-Корп", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H,
                                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    R = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!R) R = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!R) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(R, W, H);
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
    SDL_StopTextInput();

    rng.seed((unsigned)SDL_GetTicks() ^ 0x9E3779B9u ^ (unsigned)std::time(nullptr));
    initFont();
    buildTechs();
    G.techDone.assign(TECHS.size(), 0);

    char* pref = SDL_GetPrefPath("DeviceCorp", "DeviceCorp");
    if (pref) {
        savePath = std::string(pref) + "save.txt";
        SDL_free(pref);
        SDL_RWops* f = SDL_RWFromFile(savePath.c_str(), "rb");
        if (f) { saveExists = true; SDL_RWclose(f); }
    }

    if (haveAudio) {
        SDL_AudioSpec want, got;
        SDL_zero(want);
        SDL_zero(got);
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 1;
        want.samples = 1024;
        want.callback = audioCb;
        want.userdata = &A;
        A.dev = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
        if (A.dev) {
            A.rate = got.freq > 0 ? got.freq : 44100;
            SDL_PauseAudioDevice(A.dev, 0);
        }
    }

    SDL_GameController* pad = nullptr;
    if (havePad) {
        for (int i = 0; i < SDL_NumJoysticks() && !pad; ++i)
            if (SDL_IsGameController(i)) pad = SDL_GameControllerOpen(i);
    }

    Uint32 last = SDL_GetTicks();
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: running = false; break;
                case SDL_APP_WILLENTERBACKGROUND:
                    if (screen != SC_NEW && screen != SC_OVER) saveGame();
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    if (!pad && havePad) pad = SDL_GameControllerOpen(e.cdevice.which);
                    break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    if (pad && !SDL_GameControllerGetAttached(pad)) {
                        SDL_GameControllerClose(pad);
                        pad = nullptr;
                        U.pad = false;
                    }
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    switch (e.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP: uiNav(0); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: uiNav(1); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: uiNav(2); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: uiNav(3); break;
                        case SDL_CONTROLLER_BUTTON_A: uiActivate(); break;
                        case SDL_CONTROLLER_BUTTON_B: U.pad = true; goBack(); break;
                        case SDL_CONTROLLER_BUTTON_START:
                            U.pad = true;
                            if (screen == SC_PAUSE) goBack();
                            else if (screen != SC_NEW && screen != SC_OVER && !(screen == SC_DESIGN && mustDesignFirst())) setScreen(SC_PAUSE);
                            break;
                        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: G.speed = std::max(0, G.speed - 1); sfx(SFX_NAV); break;
                        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: G.speed = std::min(3, G.speed + 1); sfx(SFX_NAV); break;
                        default: break;
                    }
                    break;
                case SDL_KEYDOWN: {
                    SDL_Keycode k = e.key.keysym.sym;
                    if (textActive) {
                        if (k == SDLK_BACKSPACE) {
                            while (!newName.empty()) {
                                unsigned char c = (unsigned char)newName.back();
                                newName.pop_back();
                                if ((c & 0xC0) != 0x80) break;
                            }
                        } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_ESCAPE || k == SDLK_AC_BACK) {
                            textActive = false;
                            SDL_StopTextInput();
                        }
                        break;
                    }
                    if (k == SDLK_UP) uiNav(0);
                    else if (k == SDLK_DOWN) uiNav(1);
                    else if (k == SDLK_LEFT) uiNav(2);
                    else if (k == SDLK_RIGHT) uiNav(3);
                    else if (k == SDLK_RETURN || k == SDLK_SPACE || k == SDLK_KP_ENTER) uiActivate();
                    else if (k == SDLK_ESCAPE || k == SDLK_BACKSPACE || k == SDLK_AC_BACK) goBack();
                    break;
                }
                case SDL_TEXTINPUT:
                    if (textActive) {
                        std::string t = e.text.text;
                        if (cpLen(newName) + cpLen(t) <= 20) newName += t;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        U.pad = false;
                        U.mdown = true;
                        U.mx = U.dx0 = e.button.x;
                        U.my = U.dy0 = U.lastY = e.button.y;
                        U.dragging = false;
                        U.dragArea = areaAt(e.button.x, e.button.y);
                        U.sliderPress = false;
                        for (size_t i = 0; i < U.psl.size(); ++i)
                            if (inRect(e.button.x, e.button.y, U.psl[i])) U.sliderPress = true;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    U.mx = e.motion.x;
                    U.my = e.motion.y;
                    if (U.mdown) {
                        U.pad = false;
                        if (!U.sliderPress && !U.dragging && U.dragArea >= 0 && std::abs(U.my - U.dy0) > 10)
                            U.dragging = true;
                        if (U.dragging) scrollBy(U.dragArea, (float)(U.lastY - U.my));
                        U.lastY = U.my;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        U.mx = e.button.x;
                        U.my = e.button.y;
                        if (U.mdown && !U.dragging && !U.sliderPress) {
                            U.click = true;
                            U.cx = e.button.x;
                            U.cy = e.button.y;
                        }
                        U.mdown = false;
                        U.dragging = false;
                        U.sliderPress = false;
                    }
                    break;
                case SDL_MOUSEWHEEL: {
                    int a = areaAt(U.mx, U.my);
                    if (a >= 0) scrollBy(a, (float)(-e.wheel.y * 60));
                    break;
                }
                default: break;
            }
        }

        Uint32 now = SDL_GetTicks();
        float dt = (float)(now - last) / 1000.0f;
        last = now;
        if (dt > 0.1f) dt = 0.1f;
        uiTime += dt;
        if (toastT > 0) toastT -= dt;
        if (resIdx >= 0) resT += dt;

        if (pad) {
            float ly = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;
            float ry = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;
            uiStick(std::fabs(ly) > std::fabs(ry) ? ly : ry, dt);
        }

        bool simRunning = screen != SC_NEW && screen != SC_OVER && screen != SC_PAUSE && G.speed > 0 && resIdx < 0;
        if (simRunning) {
            const float mult[4] = {0.0f, 1.0f, 2.0f, 4.0f};
            float perDay = 1.0f / (1.5f * mult[G.speed]);
            dayTimer += dt;
            int guard = 0;
            while (dayTimer >= perDay && guard < 20 && screen != SC_OVER) {
                dayTimer -= perDay;
                tickDay();
                ++guard;
            }
            if (guard >= 20) dayTimer = 0;
        }

        renderFrame();
        uiEndFrame();
    }

    if (pad) SDL_GameControllerClose(pad);
    if (A.dev) SDL_CloseAudioDevice(A.dev);
    SDL_DestroyRenderer(R);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
