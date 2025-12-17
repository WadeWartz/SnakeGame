#include "game.h"
#include "config.h"
#include <deque>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string> // Cần cho std::string

// ===== RNG =====
static std::mt19937& Rng() { static std::mt19937 g{ std::random_device{}() }; return g; }
static int RandInt(int lo, int hi) { std::uniform_int_distribution<int> d(lo, hi); return d(Rng()); }

// ===== Trạng thái Game =====
static std::deque<Cell> gSnake;
static int  gDirX = +1, gDirY = 0;
static int  gPendX = +1, gPendY = 0;
static int  gFoodX = 20, gFoodY = 10;
static int  gScore = 0, gHighScore = 0;
static bool gPaused = false, gOver = false;

static float gMoveInterval = 0.12f;
static const float gMinInterval = 0.06f;
static float gAcc = 0.f;

static int gEvents = 0;

// Level & obstacle
static int gLevel = 1, gEatenInLevel = 0;
static std::vector<Cell> gObstacles;

// Wrap
static bool gWrapOn = WRAP_DEFAULT;

// Whirlwind
struct Whirlwind { Cell pos; float spawnTime; float lifeTime; };
static std::vector<Whirlwind> gWhirlwinds;
static float gTime = 0.f;
static bool gGhostMode = false;
static float gGhostRemaining = 0.f;
static float gWhirlwindRespawnTimer = 0.f;
static const float WHIRL_DESPAWN_TIME = 3.0f;
static const float WHIRL_RESPAWN_DELAY = 2.0f;

// ===== AUTO PILOT & GATE =====
static bool gAutoPilot = false;
static Cell gLevelGatePos{ -1, -1 };
static bool gIsExiting = false; // Trạng thái đang chui vào cổng

// High score file
static const char* SAVE_PATH = "save_highscore.txt";

// ===== PROFILE =====
static std::string gCurrentProfileName = "default";
static const char* PROFILE_LIST_FILE = "profiles.txt";

static std::string GetSaveFileName(const std::string& name) {
    return "save_" + name + ".txt";
}

// ====== UTIL ======
static bool OccupiedBySnake(int x, int y) {
    for (const auto& c : gSnake) if (c.x == x && c.y == y) return true;
    return false;
}
static bool OccupiedByObstacle(int x, int y) {
    for (const auto& c : gObstacles) if (c.x == x && c.y == y) return true;
    return false;
}
static bool OccupiedByWhirlwind(int x, int y) {
    for (const auto& w : gWhirlwinds) if (w.pos.x == x && w.pos.y == y) return true;
    return false;
}

static void SpawnObstacles(int count) {
    for (int k = 0; k < count; ++k) {
        int ox, oy, guard = 0;
        do {
            ox = RandInt(0, BOARD_W - 1);
            oy = RandInt(0, BOARD_H - 1);
            guard++;
        } while (guard < 1000 && (OccupiedBySnake(ox, oy) || OccupiedByObstacle(ox, oy) || (ox == gFoodX && oy == gFoodY)));
        if (guard < 1000) gObstacles.push_back({ ox,oy });
    }
}

static void SpawnWhirlwinds(int count) {
    for (int k = 0; k < count; ++k) {
        int wx, wy, guard = 0;
        do {
            wx = RandInt(BORDER, BOARD_W - BORDER - 1);
            wy = RandInt(BORDER, BOARD_H - BORDER - 1);
            guard++;
        } while (guard < 1000 && (OccupiedBySnake(wx, wy) || OccupiedByObstacle(wx, wy) || OccupiedByWhirlwind(wx, wy) || (wx == gFoodX && wy == gFoodY)));
        if (guard < 1000) gWhirlwinds.push_back({ {wx, wy}, gTime, WHIRL_DESPAWN_TIME });
    }
}

static void RemoveWhirlwindAt(int x, int y) {
    auto it = std::remove_if(gWhirlwinds.begin(), gWhirlwinds.end(), [&](const Whirlwind& w) { return (w.pos.x == x && w.pos.y == y); });
    gWhirlwinds.erase(it, gWhirlwinds.end());
}

static void PlaceFood() {
    if (gAutoPilot) { gFoodX = -10; gFoodY = -10; return; }
    while (true) {
        gFoodX = RandInt(0, BOARD_W - 1);
        gFoodY = RandInt(0, BOARD_H - 1);
        if (OccupiedBySnake(gFoodX, gFoodY)) continue;
        if (OccupiedByObstacle(gFoodX, gFoodY)) continue;
        if (OccupiedByWhirlwind(gFoodX, gFoodY)) continue;
        break;
    }
}

// --- KHỞI TẠO RẮN: DÙNG BIẾN INITIAL_SNAKE_LEN (5) ---
static void ResetSnake() {
    gSnake.clear();
    int sx = BOARD_W / 2, sy = BOARD_H / 2;
    // Tạo rắn dài 5 đốt ban đầu
    for (int i = 0; i < INITIAL_SNAKE_LEN; ++i) {
        gSnake.push_back({ sx - i, sy });
    }
    gDirX = +1; gDirY = 0; gPendX = +1; gPendY = 0;
}

static void UpdateWhirlwinds(float dt) {
    gWhirlwinds.erase(std::remove_if(gWhirlwinds.begin(), gWhirlwinds.end(), [&](const Whirlwind& w) { return (gTime - w.spawnTime) >= w.lifeTime; }), gWhirlwinds.end());
    if (gWhirlwinds.empty()) {
        gWhirlwindRespawnTimer += dt;
        if (gWhirlwindRespawnTimer >= WHIRL_RESPAWN_DELAY) {
            SpawnWhirlwinds(1);
            gWhirlwindRespawnTimer = 0.f;
        }
    }
    else {
        gWhirlwindRespawnTimer = 0.f;
    }
}

static void BuildLevelObstacles(int level) {
    gObstacles.clear();
    int left = 2, right = BOARD_W - 3, top = 2, bottom = BOARD_H - 3;
    if (level <= 1) return;

    if (level == 2) {
        int y1 = BOARD_H / 3, y2 = BOARD_H * 2 / 3, mid = BOARD_W / 2, gap = 3;
        for (int x = left; x <= right; ++x) {
            if (std::abs(x - mid) <= gap) continue;
            if (!OccupiedBySnake(x, y1) && !(x == gFoodX && y1 == gFoodY)) gObstacles.push_back({ x, y1 });
            if (!OccupiedBySnake(x, y2) && !(x == gFoodX && y2 == gFoodY)) gObstacles.push_back({ x, y2 });
        }
        return;
    }

    if (level >= 3) {
        int midX = BOARD_W / 2, centerY = BOARD_H / 2;
        std::vector<int> cols = { std::clamp(left + 4, 0, BOARD_W - 1), std::clamp(midX - 4, 0, BOARD_W - 1), std::clamp(midX, 0, BOARD_W - 1), std::clamp(midX + 4, 0, BOARD_W - 1), std::clamp(right - 4, 0, BOARD_W - 1) };
        for (int cx : cols) {
            int gapRadius = (cx == midX) ? 2 : 1;
            for (int y = top; y <= bottom; ++y) {
                if (std::abs(y - centerY) <= gapRadius) continue;
                if (!OccupiedBySnake(cx, y) && !(cx == gFoodX && y == gFoodY)) gObstacles.push_back({ cx, y });
            }
        }
        int shortLx = std::clamp(left + 1, 0, BOARD_W - 1);
        for (int y = top + 2; y < top + 6 && y <= bottom; ++y) if (!OccupiedBySnake(shortLx, y)) gObstacles.push_back({ shortLx, y });
        int shortRx = std::clamp(right - 1, 0, BOARD_W - 1);
        for (int y = bottom - 5; y <= bottom - 2 && y >= top; ++y) if (!OccupiedBySnake(shortRx, y)) gObstacles.push_back({ shortRx, y });
    }
}

static void StartAutoPilotToGate() {
    gAutoPilot = true;
    gLevelGatePos.x = BOARD_W - 2; // Cổng sát mép phải
    gLevelGatePos.y = BOARD_H / 2;
    gFoodX = -10; gFoodY = -10;

    gGhostMode = true;
    gGhostRemaining = 9999.f;
}

static void StartNextLevel() {
    gLevel++;
    if (gLevel > 3) gLevel = 1;

    gAutoPilot = false;
    gIsExiting = false;
    gGhostMode = false;
    gGhostRemaining = 0.f;
    gEatenInLevel = 0;

    ResetSnake();
    PlaceFood();
    BuildLevelObstacles(gLevel);

    gTime = 0.f;
    gWhirlwinds.clear();
    gWhirlwindRespawnTimer = 0.f;
    SpawnWhirlwinds(1);

    gLevelGatePos.x = -1;
}

static void UpdateAutoPilot() {
    if (!gAutoPilot || gSnake.empty()) return;
    Cell head = gSnake.front();
    int tx = gLevelGatePos.x;
    int ty = gLevelGatePos.y;

    auto applyWrap = [&](int x, int y) -> std::pair<int, int> {
        if (!gWrapOn) return { x, y };
        if (x < 0) x = BOARD_W - 1; if (x >= BOARD_W) x = 0;
        if (y < 0) y = BOARD_H - 1; if (y >= BOARD_H) y = 0;
        return { x, y };
        };

    auto isSafe = [&](int nx, int ny)->bool {
        if (nx == tx && ny == ty) return true;
        auto p = applyWrap(nx, ny);
        nx = p.first; ny = p.second;
        if (!gWrapOn && (nx < 0 || nx >= BOARD_W || ny < 0 || ny >= BOARD_H)) return false;
        if (OccupiedByObstacle(nx, ny)) return false;
        return true;
        };

    std::vector<std::pair<int, int>> allDirs = { {1,0},{-1,0},{0,1},{0,-1} };
    std::pair<int, int> reverseDir = { -gDirX, -gDirY };
    bool found = false;
    int bestDx = gDirX, bestDy = gDirY;
    int bestScore = std::numeric_limits<int>::max();

    for (auto mv : allDirs) {
        if (mv == reverseDir) continue;
        int nx = head.x + mv.first;
        int ny = head.y + mv.second;
        if (!isSafe(nx, ny)) continue;

        auto p = applyWrap(nx, ny);
        int dist = std::abs(tx - p.first) + std::abs(ty - p.second);

        if (dist < bestScore) {
            bestScore = dist; bestDx = mv.first; bestDy = mv.second; found = true;
        }
    }

    if (found) { gPendX = bestDx; gPendY = bestDy; }
    else { gPendX = gDirX; gPendY = gDirY; }
}

// ===== API =====
void Game_Init() {
    ResetSnake(); PlaceFood();
    gScore = 0; gPaused = false; gOver = false; gAcc = 0.f; gEvents = 0; gLevel = 1; gEatenInLevel = 0;
    gWrapOn = WRAP_DEFAULT;
    BuildLevelObstacles(gLevel);
    gTime = 0.f; gWhirlwinds.clear(); gGhostMode = false; gGhostRemaining = 0.f; SpawnWhirlwinds(1);
    gAutoPilot = false; gIsExiting = false;
    gLevelGatePos.x = BOARD_W - 3; gLevelGatePos.y = BOARD_H / 2;
}

void Game_Reset() { Game_Init(); }
void Game_TogglePause() { if (!gOver) gPaused = !gPaused; }
void Game_SetPaused(bool v) { if (!gOver) gPaused = v; }
void Game_RestartIfOver() { if (gOver) Game_Reset(); }

void Game_OnKeyPressed(int key) {
    if (gAutoPilot) return;
    const int K_W = 22, K_A = 0, K_S = 18, K_D = 3, K_UP = 73, K_LEFT = 71, K_DOWN = 74, K_RIGHT = 72;
    if ((key == K_W || key == K_UP) && (gDirY != +1 || gSnake.size() <= 1)) { gPendX = 0; gPendY = -1; }
    if ((key == K_S || key == K_DOWN) && (gDirY != -1 || gSnake.size() <= 1)) { gPendX = 0; gPendY = +1; }
    if ((key == K_A || key == K_LEFT) && (gDirX != +1 || gSnake.size() <= 1)) { gPendX = -1; gPendY = 0; }
    if ((key == K_D || key == K_RIGHT) && (gDirX != -1 || gSnake.size() <= 1)) { gPendX = +1; gPendY = 0; }
}

void Game_Update(float dt) {
    if (gPaused || gOver) return;
    gTime += dt;
    UpdateWhirlwinds(dt);

    if (gGhostMode && !gAutoPilot) {
        gGhostRemaining -= dt;
        if (gGhostRemaining <= 0.f) gGhostMode = false;
    }

    gAcc += dt;
    while (gAcc >= gMoveInterval) {
        gAcc -= gMoveInterval;

        if (gAutoPilot && !gIsExiting) {
            UpdateAutoPilot();
        }

        gDirX = gPendX; gDirY = gPendY;
        Cell head = gSnake.front();
        Cell next{ head.x + gDirX, head.y + gDirY };

        // === XỬ LÝ CHUI CỔNG ===
        if (gAutoPilot) {
            // 1. Chạm cổng -> Bật Exiting
            if (!gIsExiting && next.x == gLevelGatePos.x && next.y == gLevelGatePos.y) {
                gIsExiting = true;
            }

            // 2. Khi đang Exiting
            if (gIsExiting) {
                // Kiểm tra đuôi rắn đã qua cổng chưa
                if (gSnake.back().x >= gLevelGatePos.x) {
                    StartNextLevel();
                    return;
                }
            }
        }

        bool hitWall = false;
        // Chỉ check tường nếu KHÔNG phải đang thoát
        if (!gWrapOn && !gIsExiting) {
            if (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H) hitWall = true;
        }
        if (gWrapOn) {
            if (next.x < 0) next.x = BOARD_W - 1; if (next.x >= BOARD_W) next.x = 0;
            if (next.y < 0) next.y = BOARD_H - 1; if (next.y >= BOARD_H) next.y = 0;
        }

        bool hitObs = OccupiedByObstacle(next.x, next.y);
        bool hitSelf = false;

        // Không check tự cắn nếu đang thoát
        if (!gIsExiting) {
            for (const auto& c : gSnake) if (c.x == next.x && c.y == next.y) { hitSelf = true; break; }
        }

        if (!gGhostMode && !gIsExiting) {
            for (auto& w : gWhirlwinds) {
                if ((w.pos.x == next.x && w.pos.y == next.y) || (w.pos.x == head.x && w.pos.y == head.y)) {
                    gGhostMode = true; gGhostRemaining = 3.0f;
                    RemoveWhirlwindAt(w.pos.x, w.pos.y);
                    hitWall = hitObs = hitSelf = false;
                    break;
                }
            }
        }

        if (!gGhostMode && !gIsExiting) {
            if (hitWall || hitObs || hitSelf) { gOver = true; gEvents |= (1 << 1); break; }
        }
        else if (gGhostMode && !gWrapOn && hitWall && !gIsExiting) {
            if (next.x < 0) next.x = 0; else if (next.x >= BOARD_W) next.x = BOARD_W - 1;
            if (next.y < 0) next.y = 0; else if (next.y >= BOARD_H) next.y = BOARD_H - 1;
        }

        if (gOver) break;

        gSnake.push_front(next);

        // Ăn mồi (Không ăn khi đang thoát)
        if (!gIsExiting && next.x == gFoodX && next.y == gFoodY) {
            gScore++; if (gScore > gHighScore) gHighScore = gScore;
            gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.006f);

            // --- LOGIC THẮNG: SO SÁNH VỚI CHUỖI ĐÍCH ---
            // Lấy độ dài chuỗi MSSV_FULL (trong config.h, giờ là 8)
            std::string targetStr = MSSV_FULL;

            // Nếu rắn dài bằng hoặc hơn chuỗi đích -> Thắng -> AutoPilot
            if (gSnake.size() >= targetStr.length() && !gAutoPilot) {
                StartAutoPilotToGate();
            }
            else {
                PlaceFood(); // Chưa thắng -> mồi tiếp
            }
            gEvents |= (1 << 0);
        }
        else {
            gSnake.pop_back();
        }
    }
}

// Getters & Setters
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
bool Game_IsGhost() { return gGhostMode; }
std::size_t Game_WhirlwindCount() { return gWhirlwinds.size(); }
Cell Game_Whirlwind(std::size_t i) { return gWhirlwinds[i].pos; }
bool Game_IsAutoPilot() { return gAutoPilot; }
Cell Game_PortalPos() { return gLevelGatePos; }
bool Game_IsExiting() { return gIsExiting; }
int Game_ConsumeEvents() { int e = gEvents; gEvents = 0; return e; }
void Game_ToggleWrap() { if (!gOver) gWrapOn = !gWrapOn; }
bool Game_WrapOn() { return gWrapOn; }

// Save/Load
void Game_LoadHighScore() { std::ifstream fin(SAVE_PATH); if (fin) fin >> gHighScore; }
void Game_SaveHighScore() { static int l = -1; if (gHighScore == l) return; std::ofstream f(SAVE_PATH, std::ios::trunc); if (f) { f << gHighScore; l = gHighScore; } }
int Game_HighScore() { return gHighScore; }

void Game_SetProfileName(const std::string& name) { gCurrentProfileName = name; }
std::string Game_GetProfileName() { return gCurrentProfileName; }
std::vector<std::string> Game_GetProfileList() {
    std::vector<std::string> list; std::ifstream in(PROFILE_LIST_FILE); std::string line;
    while (std::getline(in, line)) if (!line.empty()) list.push_back(line);
    return list;
}
bool Game_CreateProfile(const std::string& name) {
    if (name.empty()) return false; auto list = Game_GetProfileList();
    for (const auto& s : list) if (s == name) return false;
    std::ofstream out(PROFILE_LIST_FILE, std::ios::app); out << name << "\n"; return true;
}
std::string Game_GetProfileInfo(const std::string& name) {
    std::string fn = GetSaveFileName(name); std::ifstream in(fn); if (!in) return "New";
    int s, h, l; if (in >> s >> h >> l) return "Lv:" + std::to_string(l) + " Sc:" + std::to_string(s);
    return "Err";
}
bool Game_SaveGame() {
    std::string filename = GetSaveFileName(gCurrentProfileName);
    std::ofstream out(filename); if (!out) return false;
    out << gScore << ' ' << gHighScore << ' ' << gLevel << ' ' << gEatenInLevel << ' ' << gMoveInterval << ' '
        << gDirX << ' ' << gDirY << ' ' << gPendX << ' ' << gPendY << ' ' << (int)gWrapOn << '\n';
    out << gFoodX << ' ' << gFoodY << '\n';
    out << gSnake.size() << '\n'; for (const auto& c : gSnake) out << c.x << ' ' << c.y << '\n';
    out << gObstacles.size() << '\n'; for (const auto& o : gObstacles) out << o.x << ' ' << o.y << '\n';
    return true;
}
bool Game_LoadGame() {
    std::string filename = GetSaveFileName(gCurrentProfileName);
    std::ifstream in(filename); if (!in) return false;
    int score, hi, level, eatenLv; float moveInt; int dirX, dirY, pendX, pendY; int wrapOn;
    if (!(in >> score >> hi >> level >> eatenLv >> moveInt >> dirX >> dirY >> pendX >> pendY >> wrapOn)) return false;
    int foodX, foodY; if (!(in >> foodX >> foodY)) return false;
    size_t snakeLen; if (!(in >> snakeLen)) return false;
    std::deque<Cell> snakeTmp;
    for (size_t i = 0; i < snakeLen; ++i) { Cell c; if (!(in >> c.x >> c.y)) return false; snakeTmp.push_back(c); }
    size_t obsLen; if (!(in >> obsLen)) return false;
    std::vector<Cell> obsTmp;
    for (size_t i = 0; i < obsLen; ++i) { Cell o; if (!(in >> o.x >> o.y)) return false; obsTmp.push_back(o); }
    gScore = score; gHighScore = hi; gLevel = level; gEatenInLevel = eatenLv; gMoveInterval = moveInt;
    gDirX = dirX; gDirY = dirY; gPendX = pendX; gPendY = pendY; gWrapOn = (wrapOn != 0);
    gFoodX = foodX; gFoodY = foodY; gSnake = std::move(snakeTmp); gObstacles = std::move(obsTmp);
    gPaused = false; gOver = false; gAcc = 0.f; gEvents = 0;
    return true;
}