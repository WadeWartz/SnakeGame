#include "game.h"
#include "config.h"
#include <deque>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>

// ===== RNG =====
static std::mt19937& Rng() { static std::mt19937 g{ std::random_device{}() }; return g; }
static int RandInt(int lo, int hi) { std::uniform_int_distribution<int> d(lo, hi); return d(Rng()); }

// ===== Trạng thái =====
static std::deque<Cell> gSnake;      // [0] = đầu
static int  gDirX = +1, gDirY = 0;
static int  gPendX = +1, gPendY = 0;
static int  gFoodX = 20, gFoodY = 10;
static int  gScore = 0, gHighScore = 0;
static bool gPaused = false, gOver = false;

static float gMoveInterval = 0.12f;     // giây/bước
static const float gMinInterval = 0.06f;
static float gAcc = 0.f;

static int gEvents = 0; // bit0=EAT, bit1=DIE

// Level & obstacle
static int gLevel = 1, gEatenInLevel = 0;
static std::vector<Cell> gObstacles;

// Portal & Wrap
static bool gWrapOn = WRAP_DEFAULT;
static bool gPortalActive = false;
static Cell gPortalA{ 0,0 }, gPortalB{ BOARD_W - 1, BOARD_H - 1 };

// High score file
static const char* SAVE_PATH = "save_highscore.txt";

// ====== UTIL (TRÊN Game_Update) ======
static bool OccupiedBySnake(int x, int y) {
    for (const auto& c : gSnake) if (c.x == x && c.y == y) return true;
    return false;
}
static bool OccupiedByObstacle(int x, int y) {
    for (const auto& c : gObstacles) if (c.x == x && c.y == y) return true;
    return false;
}
static void SpawnObstacles(int count) {
    for (int k = 0; k < count; ++k) {
        int ox, oy, guard = 0;
        do {
            ox = RandInt(0, BOARD_W - 1);
            oy = RandInt(0, BOARD_H - 1);
            guard++;
        } while (guard < 1000 && (OccupiedBySnake(ox, oy) ||
            OccupiedByObstacle(ox, oy) ||
            (ox == gFoodX && oy == gFoodY)));
        if (guard < 1000) gObstacles.push_back({ ox,oy });
    }
}
static void SpawnPortals() {
    auto ok = [&](int x, int y) {
        if (OccupiedBySnake(x, y)) return false;
        if (OccupiedByObstacle(x, y)) return false;
        if (x == gFoodX && y == gFoodY) return false;
        return true;
        };
    int guard = 0;
    do {
        gPortalA.x = RandInt(0, BOARD_W - 1);
        gPortalA.y = RandInt(0, BOARD_H - 1);
        guard++;
    } while (guard < 1000 && !ok(gPortalA.x, gPortalA.y));
    guard = 0;
    do {
        gPortalB.x = RandInt(0, BOARD_W - 1);
        gPortalB.y = RandInt(0, BOARD_H - 1);
        guard++;
    } while (guard < 1000 && (!ok(gPortalB.x, gPortalB.y) ||
        (gPortalB.x == gPortalA.x && gPortalB.y == gPortalA.y)));
}
static void PlaceFood() {
    while (true) {
        gFoodX = RandInt(0, BOARD_W - 1);
        gFoodY = RandInt(0, BOARD_H - 1);
        if (OccupiedBySnake(gFoodX, gFoodY)) continue;
        if (OccupiedByObstacle(gFoodX, gFoodY)) continue;
        if (gPortalActive && ((gFoodX == gPortalA.x && gFoodY == gPortalA.y) ||
            (gFoodX == gPortalB.x && gFoodY == gPortalB.y))) continue;
        break;
    }
}
static void ResetSnake() {
    gSnake.clear();
    int sx = BOARD_W / 2, sy = BOARD_H / 2;
    for (int i = 0; i < 5; ++i) gSnake.push_back({ sx - i, sy }); // head = front = (sx,sy)
    gDirX = +1; gDirY = 0; gPendX = +1; gPendY = 0;
}

// ===== High score =====
void Game_LoadHighScore() {
    std::ifstream fin(SAVE_PATH);
    if (!fin) { gHighScore = 0; return; }
    fin >> gHighScore;
    if (!fin) gHighScore = 0;
}
void Game_SaveHighScore() {
    static int last = -1;
    if (gHighScore == last) return;
    std::ofstream fout(SAVE_PATH, std::ios::trunc);
    if (fout) { fout << gHighScore; last = gHighScore; }
}
int Game_HighScore() { return gHighScore; }

// ===== API vòng đời & input =====
void Game_Init() {
    ResetSnake(); PlaceFood();
    gScore = 0; gPaused = false; gOver = false;
    gMoveInterval = 0.12f; gAcc = 0.f; gEvents = 0;
    gLevel = 1; gEatenInLevel = 0; gObstacles.clear();
    gWrapOn = WRAP_DEFAULT;
    gPortalActive = (gLevel >= PORTAL_MIN_LEVEL);
    if (gPortalActive) SpawnPortals();
}
void Game_Reset() {
    ResetSnake(); PlaceFood();
    gScore = 0; gPaused = false; gOver = false;
    gMoveInterval = 0.12f; gAcc = 0.f; gEvents = 0;
    gLevel = 1; gEatenInLevel = 0; gObstacles.clear();
    gWrapOn = WRAP_DEFAULT;
    gPortalActive = false; // level 1
}
void Game_TogglePause() { if (!gOver) gPaused = !gPaused; }
void Game_SetPaused(bool v) { if (!gOver) gPaused = v; }
void Game_RestartIfOver() { if (gOver) Game_Reset(); }

void Game_OnKeyPressed(int key) {
    // Giá trị enum sf::Keyboard (SFML 2.6.x)
    const int K_W = 22, K_A = 0, K_S = 18, K_D = 3, K_UP = 73, K_LEFT = 71, K_DOWN = 74, K_RIGHT = 72;
    if (key == K_W || key == K_UP) { if (gDirY != +1 || gSnake.size() <= 1) { gPendX = 0;  gPendY = -1; } }
    if (key == K_S || key == K_DOWN) { if (gDirY != -1 || gSnake.size() <= 1) { gPendX = 0;  gPendY = +1; } }
    if (key == K_A || key == K_LEFT) { if (gDirX != +1 || gSnake.size() <= 1) { gPendX = -1; gPendY = 0; } }
    if (key == K_D || key == K_RIGHT) { if (gDirX != -1 || gSnake.size() <= 1) { gPendX = +1; gPendY = 0; } }
}

void Game_Update(float dt) {
    if (gPaused || gOver) return;
    gAcc += dt;
    while (gAcc >= gMoveInterval) {
        gAcc -= gMoveInterval;

        // áp hướng pending
        gDirX = gPendX; gDirY = gPendY;

        Cell head = gSnake.front();
        Cell next{ head.x + gDirX, head.y + gDirY };

        // WRAP
        if (gWrapOn) {
            if (next.x < 0) next.x = BOARD_W - 1;
            if (next.x >= BOARD_W) next.x = 0;
            if (next.y < 0) next.y = BOARD_H - 1;
            if (next.y >= BOARD_H) next.y = 0;
        }
        else {
            if (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H) {
                gOver = true; gEvents |= (1 << 1); break;
            }
        }

        // Portal
        if (gPortalActive) {
            if (next.x == gPortalA.x && next.y == gPortalA.y) next = gPortalB;
            else if (next.x == gPortalB.x && next.y == gPortalB.y) next = gPortalA;
        }

        // Obstacle
        if (OccupiedByObstacle(next.x, next.y)) { gOver = true; gEvents |= (1 << 1); break; }

        // Tự va
        for (const auto& c : gSnake) { if (c.x == next.x && c.y == next.y) { gOver = true; gEvents |= (1 << 1); break; } }
        if (gOver) break;

        gSnake.push_front(next);

        // Ăn mồi?
        if (next.x == gFoodX && next.y == gFoodY) {
            gScore += 1;
            if (gScore > gHighScore) gHighScore = gScore;
            gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.006f);
            PlaceFood();

            gEatenInLevel++;
            if (gEatenInLevel >= LEVEL_STEP) {
                gLevel++; gEatenInLevel = 0;
                SpawnObstacles(OBSTACLE_PER_LV);
                gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.01f);

                if (!gPortalActive && gLevel >= PORTAL_MIN_LEVEL) {
                    gPortalActive = true;
                    SpawnPortals();
                    if (gFoodX == gPortalA.x && gFoodY == gPortalA.y) PlaceFood();
                    if (gFoodX == gPortalB.x && gFoodY == gPortalB.y) PlaceFood();
                }
            }

            gEvents |= (1 << 0); // EAT
            // không pop đuôi
        }
        else {
            gSnake.pop_back();
        }
    }
}

// ===== Getters =====
int Game_Score() { return gScore; }
bool Game_Paused() { return gPaused; }
bool Game_Over() { return gOver; }
float Game_MoveInterval() { return gMoveInterval; }
int Game_Level() { return gLevel; }

std::size_t Game_SnakeLen() { return gSnake.size(); }
Cell Game_SnakeSeg(std::size_t i) { return gSnake[i]; }
Cell Game_Food() { return { gFoodX,gFoodY }; }

std::size_t Game_ObstacleCount() { return gObstacles.size(); }
Cell Game_Obstacle(std::size_t i) { return gObstacles[i]; }

int Game_ConsumeEvents() { int e = gEvents; gEvents = 0; return e; }

// Wrap & Portal getters/toggles
void Game_ToggleWrap() { if (!gOver) gWrapOn = !gWrapOn; }
bool Game_WrapOn() { return gWrapOn; }

bool Game_PortalsActive() { return gPortalActive; }
Cell Game_PortalA() { return gPortalA; }
Cell Game_PortalB() { return gPortalB; }
