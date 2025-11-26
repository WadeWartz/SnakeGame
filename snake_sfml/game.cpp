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

int g_Level = 1;
PortalU* g_Ugate = nullptr;

static bool g_Transitioning = false;
static sf::Texture g_TransitionTex;
static sf::Sprite g_TransitionSprite;
static float g_TransitionAlpha = 0.f;

// Level & obstacle
static int gLevel = 1, gEatenInLevel = 0;
static std::vector<Cell> gObstacles;

//whirlwind biến mất sau 5s
struct Whirlwind {
    Cell pos;
    float spawnTime; // thời điểm sinh, tính bằng tổng thời gian game
};

// Whirlwinds (lốc xoáy)
//static std::vector<Cell> gWhirlwinds;
static std::vector<Whirlwind> gWhirlwinds;
static float gTime = 0.f; // tổng thời gian game

static int gEatenSinceWhirl = 0; // đếm đồ ăn để spawn whirlwind mỗi 4 đồ ăn

// Portal & Wrap
//static bool gWrapOn = WRAP_DEFAULT;
//static bool gPortalActive = false;
//static Cell gPortalA{ 0,0 }, gPortalB{ BOARD_W - 1, BOARD_H - 1 };

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

static bool OccupiedByWhirlwind(int x, int y) {
    for (const auto& w : gWhirlwinds)
        if (w.pos.x == x && w.pos.y == y) return true; // <-- sửa w.x -> w.pos.x
    return false;
}

static void RemoveWhirlwindAt(int x, int y) {
    for (auto it = gWhirlwinds.begin(); it != gWhirlwinds.end(); ++it) {
        if (it->pos.x == x && it->pos.y == y) { // <-- sửa it->x -> it->pos.x
            gWhirlwinds.erase(it);
            return;
        }
    }
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
            (ox == gFoodX && oy == gFoodY) ||
            OccupiedByWhirlwind(ox, oy)));
        if (guard < 1000) gObstacles.push_back({ ox,oy });
    }
}

//static void SpawnPortals() {
//    auto ok = [&](int x, int y) {
//        if (OccupiedBySnake(x, y)) return false;
//        if (OccupiedByObstacle(x, y)) return false;
//        if (OccupiedByWhirlwind(x, y)) return false;
//        if (x == gFoodX && y == gFoodY) return false;
//        return true;
//        };
//    int guard = 0;
//    do {
//        gPortalA.x = RandInt(0, BOARD_W - 1);
//        gPortalA.y = RandInt(0, BOARD_H - 1);
//        guard++;
//    } while (guard < 1000 && !ok(gPortalA.x, gPortalA.y));
//    guard = 0;
//    do {
//        gPortalB.x = RandInt(0, BOARD_W - 1);
//        gPortalB.y = RandInt(0, BOARD_H - 1);
//        guard++;
//    } while (guard < 1000 && (!ok(gPortalB.x, gPortalB.y) ||
//        (gPortalB.x == gPortalA.x && gPortalB.y == gPortalA.y)));
//}

// Spawn whirl (lốc xoáy) — chọn ô trống (không phải snake, obstacle, food, portal, whirl)
static void SpawnWhirlwind() {
    std::vector<Cell> freeCells;
    for (int y = 0; y < BOARD_H; ++y) {
        for (int x = 0; x < BOARD_W; ++x) {
            if (OccupiedBySnake(x, y)) continue;
            if (OccupiedByObstacle(x, y)) continue;
            if (OccupiedByWhirlwind(x, y)) continue;
            //if (gPortalActive && ((x == gPortalA.x && y == gPortalA.y) || (x == gPortalB.x && y == gPortalB.y))) continue;
            if (x == gFoodX && y == gFoodY) continue;
            freeCells.push_back({ x,y });
        }
    }
    if (freeCells.empty()) return;
    int idx = RandInt(0, (int)freeCells.size() - 1);
    //gWhirlwinds.push_back(freeCells[idx]);
    Whirlwind w;
    w.pos = freeCells[idx];
    w.spawnTime = gTime;   // lưu thời điểm sinh
    gWhirlwinds.push_back(w);
}

// PlaceFood tránh whirlwinds
static void PlaceFood() {
    while (true) {
        gFoodX = RandInt(0, BOARD_W - 1);
        gFoodY = RandInt(0, BOARD_H - 1);
        if (OccupiedBySnake(gFoodX, gFoodY)) continue;
        if (OccupiedByObstacle(gFoodX, gFoodY)) continue;
        if (OccupiedByWhirlwind(gFoodX, gFoodY)) continue;
        /*if (gPortalActive && ((gFoodX == gPortalA.x && gFoodY == gPortalA.y) ||
            (gFoodX == gPortalB.x && gFoodY == gPortalB.y))) continue;*/
        break;
    }
}

static void ResetSnake() {
    gSnake.clear();
    int sx = BOARD_W / 2, sy = BOARD_H / 2;
    for (int i = 0; i < 5; ++i) gSnake.push_back({ sx - i, sy }); // head = front = (sx,sy)
    gDirX = +1; gDirY = 0; gPendX = +1; gPendY = 0;
    // note: whirlwinds cleared in Game_Init/Game_Reset
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
    gWhirlwinds.clear(); gEatenSinceWhirl = 0;
    //gWrapOn = WRAP_DEFAULT;
    /*gPortalActive = (gLevel >= PORTAL_MIN_LEVEL);
    if (gPortalActive) SpawnPortals();*/
}

void Game_Reset() {
    ResetSnake(); PlaceFood();
    gScore = 0; gPaused = false; gOver = false;
    gMoveInterval = 0.12f; gAcc = 0.f; gEvents = 0;
    gLevel = 1; gEatenInLevel = 0; gObstacles.clear();
    gWhirlwinds.clear(); gEatenSinceWhirl = 0;
    //gWrapOn = WRAP_DEFAULT;
    //gPortalActive = false; // level 1
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
    gTime += dt;
    gAcc += dt;

    // Xoá các whirlwinds đã tồn tại hơn 5 giây
    for (size_t i = 0; i < gWhirlwinds.size(); ) {
        if (gTime - gWhirlwinds[i].spawnTime >= 5.f)
            gWhirlwinds.erase(gWhirlwinds.begin() + i);
        else
            i++;
    }

    while (gAcc >= gMoveInterval) {
        gAcc -= gMoveInterval;

        // áp hướng pending
        gDirX = gPendX; gDirY = gPendY;

        Cell head = gSnake.front();
        Cell next{ head.x + gDirX, head.y + gDirY };

        //// WRAP
        //if (gWrapOn) {
        //    if (next.x < 0) next.x = BOARD_W - 1;
        //    if (next.x >= BOARD_W) next.x = 0;
        //    if (next.y < 0) next.y = BOARD_H - 1;
        //    if (next.y >= BOARD_H) next.y = 0;
        //}
        //else {
        //    if (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H) {
        //        gOver = true; gEvents |= (1 << 1); break;
        //    }
        //}

        // Biên giới
        if (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H) {
            gOver = true; gEvents |= (1 << 1);
            break;
        }

        //// Portal
        //if (gPortalActive) {
        //    if (next.x == gPortalA.x && next.y == gPortalA.y) next = gPortalB;
        //    else if (next.x == gPortalB.x && next.y == gPortalB.y) next = gPortalA;
        //}

        // Obstacle (rock etc)
        if (OccupiedByObstacle(next.x, next.y)) { gOver = true; gEvents |= (1 << 1); break; }

        // --- Whirlwind: nếu next là whirl, xử lý đặc biệt (xoay hướng 90°) ---
        if (OccupiedByWhirlwind(next.x, next.y)) {
            // remove the whirl we hit
            RemoveWhirlwindAt(next.x, next.y);
            //đụng là làm rắn di chuyển chậm
            gMoveInterval = std::min(gMoveInterval + 0.03f, 0.25f);

            // rotate current direction 90 degrees clockwise:
            // newDx = -oldDy; newDy = oldDx;
            int oldDx = gDirX, oldDy = gDirY;
            int newDx = -oldDy, newDy = oldDx;
            gDirX = newDx; gDirY = newDy;
            gPendX = newDx; gPendY = newDy; // also update pending so next tick consistent

            // recompute next cell after rotation
            next = { head.x + gDirX, head.y + gDirY };

            //// apply wrap again
            //if (gWrapOn) {
            //    if (next.x < 0) next.x = BOARD_W - 1;
            //    if (next.x >= BOARD_W) next.x = 0;
            //    if (next.y < 0) next.y = BOARD_H - 1;
            //    if (next.y >= BOARD_H) next.y = 0;
            //}
            //else {
            //    if (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H) {
            //        gOver = true; gEvents |= (1 << 1); break;
            //    }
            //}

        //    // Portal again (in case rotated into portal)
        //    if (gPortalActive) {
        //        if (next.x == gPortalA.x && next.y == gPortalA.y) next = gPortalB;
        //        else if (next.x == gPortalB.x && next.y == gPortalB.y) next = gPortalA;
        //    }
        }

        // logic bình thường: di chuyển rắn, ăn táo...

        // Tự va
        for (const auto& c : gSnake) { if (c.x == next.x && c.y == next.y) { gOver = true; gEvents |= (1 << 1); break; } }
        if (gOver) break;

        gSnake.push_front(next);

        // Ăn mồi?
        if (next.x == gFoodX && next.y == gFoodY) {
            gScore += 1;
            if (gScore > gHighScore) gHighScore = gScore;
            gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.006f);

            // tăng counter để spawn whirl mỗi 4 đồ ăn
            gEatenSinceWhirl++;
            if (gEatenSinceWhirl >= 4) {
                gEatenSinceWhirl = 0;
                SpawnWhirlwind();
            }

            PlaceFood();

            gEatenInLevel++;

            if (gEatenInLevel >= LEVEL_STEP) {
                gLevel++; gEatenInLevel = 0;
                SpawnObstacles(OBSTACLE_PER_LV);
                gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.01f);

                /*if (!gPortalActive && gLevel >= PORTAL_MIN_LEVEL) {
                    gPortalActive = true;
                    SpawnPortals();
                    if (gFoodX == gPortalA.x && gFoodY == gPortalA.y) PlaceFood();
                    if (gFoodX == gPortalB.x && gFoodY == gPortalB.y) PlaceFood();
                }*/
            }

            gEvents |= (1 << 0); // EAT
            // không pop đuôi
        }
        else {
            gSnake.pop_back();
        }
    }

    // Transition chỉ update alpha, không return
    if (g_Transitioning) {
        g_TransitionAlpha += 300 * dt;  // fade theo thời gian

        if (g_TransitionAlpha >= 255) {
            g_TransitionAlpha = 255;
            g_Transitioning = false;
        }

        g_TransitionSprite.setColor(
            sf::Color(255, 255, 255, (int)g_TransitionAlpha)
        );
    }
}

//void Game_CreateUGate() {
//    if (g_Ugate) { delete g_Ugate; g_Ugate = nullptr; }
//
//    // ví dụ đặt cổng ở góc phải map
//    sf::Vector2i gatePos = { BOARD_W - 2, BOARD_H / 2 };
//
//    g_Ugate = new PortalU("assets/portalU.png", gatePos, TILE);
//}

//void Game_NextLevel() {
//    g_Level++;
//
//    // reset snake, map, đồ ăn …
//    Game_Reset();
//
//    // tạo lại cổng chữ U
//    Game_CreateUGate();
//}

//void Game_TriggerTransition() {
//    g_Transitioning = true;
//    g_TransitionAlpha = 0.f;
//
//    // load ảnh chuyển cảnh cho level mới
//    std::string path = "assets/transition" + std::to_string(g_Level) + ".png";
//    g_TransitionTex.loadFromFile(path);
//    g_TransitionSprite.setTexture(g_TransitionTex);
//    g_TransitionSprite.setPosition(0, 0);
//}

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

//// Whirlwind getters
//std::size_t Game_WhirlwindCount() { return gWhirlwinds.size(); }
//Cell Game_Whirlwind(std::size_t i) { return gWhirlwinds[i]; }

// Whirlwind getters
std::size_t Game_WhirlwindCount() { return gWhirlwinds.size(); }
Cell Game_Whirlwind(std::size_t i) { return gWhirlwinds[i].pos; }

int Game_ConsumeEvents() { int e = gEvents; gEvents = 0; return e; }

//// Wrap & Portal getters/toggles
//void Game_ToggleWrap() { if (!gOver) gWrapOn = !gWrapOn; }
//bool Game_WrapOn() { return gWrapOn; }

//bool Game_PortalsActive() { return gPortalActive; }
//Cell Game_PortalA() { return gPortalA; }
//Cell Game_PortalB() { return gPortalB; }
