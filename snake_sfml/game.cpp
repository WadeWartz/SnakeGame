#include "game.h"
#include "config.h"
#include <deque>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream> // Cần thêm thư viện này để xử lý chuỗi

// ===== RNG =====
static std::mt19937& Rng() { static std::mt19937 g{ std::random_device{}() }; return g; }
static int RandInt(int lo, int hi) { std::uniform_int_distribution<int> d(lo, hi); return d(Rng()); }

// ===== Trạng thái Game =====
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

// Wrap
static bool gWrapOn = WRAP_DEFAULT;

struct Whirlwind { Cell pos; float spawnTime; float lifeTime; };
static std::vector<Whirlwind> gWhirlwinds;
static float gTime = 0.f;
static bool gGhostMode = false;
static float gGhostRemaining = 0.f;
static float gWhirlwindRespawnTimer = 0.f;
static const float WHIRL_DESPAWN_TIME = 3.0f;
static const float WHIRL_RESPAWN_DELAY = 2.0f;
// ===== AUTO PILOT & CỔNG QUA MÀN =====
static bool gAutoPilot = false;

// Vị trí cổng Torii để qua màn (từng level sẽ set lại)
static Cell gLevelGatePos{ -1, -1 };
// High score file (Global Score - giữ nguyên hoặc bỏ tùy bạn)
static const char* SAVE_PATH = "save_highscore.txt";

// =========================================================
// ===== QUẢN LÝ PROFILE (BIẾN TOÀN CỤC MỚI) =====
// =========================================================
static std::string gCurrentProfileName = "default"; // Tên người chơi hiện tại
static const char* PROFILE_LIST_FILE = "profiles.txt"; // File chứa danh sách tên người chơi

// Hàm nội bộ: Tạo tên file save dựa trên tên người chơi
// Ví dụ: tên là "Dava" -> file "save_Dava.txt"
static std::string GetSaveFileName(const std::string& name) {
    return "save_" + name + ".txt";
}

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
    for (const auto& w : gWhirlwinds) {
        if (w.pos.x == x && w.pos.y == y) return true;
    }
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

static void SpawnWhirlwinds(int count) {
    for (int k = 0; k < count; ++k) {
        int wx, wy, guard = 0;
        do {
            wx = RandInt(BORDER, BOARD_W - BORDER - 1);
            wy = RandInt(BORDER, BOARD_H - BORDER - 1);
            guard++;
        } while (guard < 1000 &&
            (OccupiedBySnake(wx, wy) ||
                OccupiedByObstacle(wx, wy) ||
                OccupiedByWhirlwind(wx, wy) ||
                (wx == gFoodX && wy == gFoodY)));

        if (guard < 1000) {
            gWhirlwinds.push_back({ {wx, wy}, gTime, WHIRL_DESPAWN_TIME });
        }
    }
}
// Xoá whirlwind tại vị trí (x, y)
static void RemoveWhirlwindAt(int x, int y)
{
    auto it = std::remove_if(
        gWhirlwinds.begin(),
        gWhirlwinds.end(),
        [&](const Whirlwind& w) {
            return (w.pos.x == x && w.pos.y == y);
        }
    );

    gWhirlwinds.erase(it, gWhirlwinds.end());
}

static void PlaceFood() {
    while (true) {
        gFoodX = RandInt(0, BOARD_W - 1);
        gFoodY = RandInt(0, BOARD_H - 1);
        if (OccupiedBySnake(gFoodX, gFoodY)) continue;
        if (OccupiedByObstacle(gFoodX, gFoodY)) continue;
        if (OccupiedByWhirlwind(gFoodX, gFoodY)) continue; // whirlwinds still considered
        break;
    }
}
static void ResetSnake() {
    gSnake.clear();
    int sx = BOARD_W / 2, sy = BOARD_H / 2;
    for (int i = 0; i < 5; ++i) gSnake.push_back({ sx - i, sy }); // head = front = (sx,sy)
    gDirX = +1; gDirY = 0; gPendX = +1; gPendY = 0;
}

static void UpdateWhirlwinds(float dt)
{
    // 1) Xóa whirlwind đã quá thời gian sống
    gWhirlwinds.erase(
        std::remove_if(
            gWhirlwinds.begin(),
            gWhirlwinds.end(),
            [&](const Whirlwind& w) {
                return (gTime - w.spawnTime) >= w.lifeTime;
            }
        ),
        gWhirlwinds.end()
    );

    // 2) Nếu không còn whirlwind nào → đếm thời gian chờ spawn lại
    if (gWhirlwinds.empty()) {
        gWhirlwindRespawnTimer += dt;

        // 3) Sau khi chờ đủ → spawn lại
        if (gWhirlwindRespawnTimer >= WHIRL_RESPAWN_DELAY) {
            SpawnWhirlwinds(1);      // số lượng bạn có thể chỉnh
            gWhirlwindRespawnTimer = 0.f;
        }
    }
    else {
        // Nếu vẫn còn whirlwind → reset timer
        gWhirlwindRespawnTimer = 0.f;
    }
}

// Xây obstacle cố định cho từng level (3 màn chính)
static void BuildLevelObstacles(int level)
{
    gObstacles.clear();

    // Biên an toàn, tránh sát mép
    int left = 2;
    int right = BOARD_W - 3;
    int top = 2;
    int bottom = BOARD_H - 3;

    // Level 1: không có chướng ngại (màn luyện cơ bản)
    if (level <= 1) {
        return;
    }

    // Level 2: Hai bức tường ngang trên và dưới, chừa lỗ ở giữa
    if (level == 2) {
        int y1 = BOARD_H / 3;
        int y2 = BOARD_H * 2 / 3;
        int mid = BOARD_W / 2;
        int gap = 3; // độ rộng lỗ trống

        for (int x = left; x <= right; ++x) {
            if (std::abs(x - mid) <= gap) continue; // chừa lỗ ở giữa

            if (!OccupiedBySnake(x, y1) && !(x == gFoodX && y1 == gFoodY))
                gObstacles.push_back({ x, y1 });

            if (!OccupiedBySnake(x, y2) && !(x == gFoodX && y2 == gFoodY))
                gObstacles.push_back({ x, y2 });
        }
        return;
    }

    // Level >= 3: multiple vertical columns (adjusted to match image)
    if (level >= 3) {
        int midX = BOARD_W / 2;
        int centerY = BOARD_H / 2;

        // Column X positions (left -> left-center -> center -> right-center -> right)
        std::vector<int> cols;
        cols.push_back(std::clamp(left + 4, 0, BOARD_W - 1));
        cols.push_back(std::clamp(midX - 4, 0, BOARD_W - 1));
        cols.push_back(std::clamp(midX, 0, BOARD_W - 1));
        cols.push_back(std::clamp(midX + 4, 0, BOARD_W - 1));
        cols.push_back(std::clamp(right - 4, 0, BOARD_W - 1));

        for (int cx : cols) {
            // Make each column tall but leave a short gap around center for navigation
            int gapRadius = (cx == midX) ? 2 : 1; // larger gap in center column
            for (int y = top; y <= bottom; ++y) {
                if (std::abs(y - centerY) <= gapRadius) continue; // central gap
                if (!OccupiedBySnake(cx, y) && !(cx == gFoodX && y == gFoodY))
                    gObstacles.push_back({ cx, y });
            }
        }

        // Add two short vertical stacks (upper-left and lower-right) like in the photo
        int shortLx = std::clamp(left + 1, 0, BOARD_W - 1);
        for (int y = top + 2; y < top + 6 && y <= bottom; ++y) {
            if (!OccupiedBySnake(shortLx, y) && !(shortLx == gFoodX && y == gFoodY))
                gObstacles.push_back({ shortLx, y });
        }

        int shortRx = std::clamp(right - 1, 0, BOARD_W - 1);
        for (int y = bottom - 5; y <= bottom - 2 && y >= top; ++y) {
            if (!OccupiedBySnake(shortRx, y) && !(shortRx == gFoodX && y == gFoodY))
                gObstacles.push_back({ shortRx, y });
        }

        return;
    }
}

// Bắt đầu chế độ AutoPilot: rắn tự chạy tới cổng Torii
static void StartAutoPilotToGate()
{
    gAutoPilot = true;

    // Cổng nằm gần mép phải, giữa màn (mỗi level dùng chung vị trí này)
    gLevelGatePos.x = BOARD_W - 3;
    gLevelGatePos.y = BOARD_H / 2;

    // Ẩn mồi, để rắn chỉ tập trung chạy tới cổng
    gFoodX = -10;
    gFoodY = -10;

    // Bật ghost mode "vô hạn" trong lúc auto (để không chết khi đụng)
    gGhostMode = true;
    gGhostRemaining = 9999.f;
}
// Khi rắn chạm cổng Torii -> qua màn mới
static void StartNextLevel()
{
    // Tăng level, quay vòng 1..3
    gLevel++;
    if (gLevel > 3) gLevel = 1;

    gAutoPilot = false;
    gGhostMode = false;
    gGhostRemaining = 0.f;
    gEatenInLevel = 0;

    // Reset trạng thái game
    ResetSnake();
    PlaceFood();

    // Xây obstacle theo level mới
    BuildLevelObstacles(gLevel);

    // Reset whirlwind
    gTime = 0.f;
    gWhirlwinds.clear();
    gWhirlwindRespawnTimer = 0.f;
    SpawnWhirlwinds(1);
}
// Điều chỉnh gPendX, gPendY để rắn tự đi về cổng
static void UpdateAutoPilot()
{
    if (!gAutoPilot || gSnake.empty()) return;

    Cell head = gSnake.front();
    int tx = gLevelGatePos.x;
    int ty = gLevelGatePos.y;

    auto sgn = [](int v) { return (v > 0) ? 1 : ((v < 0) ? -1 : 0); };

    // helper: apply wrap if enabled
    auto applyWrap = [&](int x, int y) -> std::pair<int, int> {
        if (!gWrapOn) return { x, y };
        if (x < 0) x = BOARD_W - 1;
        if (x >= BOARD_W) x = 0;
        if (y < 0) y = BOARD_H - 1;
        if (y >= BOARD_H) y = 0;
        return { x, y };
        };

    // helper: test if a cell would be safe to step into (allow stepping onto gate)
    auto isSafe = [&](int nx, int ny)->bool {
        // gate is always allowed
        if (nx == tx && ny == ty) return true;
        auto p = applyWrap(nx, ny);
        nx = p.first; ny = p.second;
        // out of bounds when wrap is off
        if (!gWrapOn) {
            if (nx < 0 || nx >= BOARD_W || ny < 0 || ny >= BOARD_H) return false;
        }
        if (OccupiedByObstacle(nx, ny)) return false;
        if (OccupiedBySnake(nx, ny)) return false;
        // whirlwinds/food don't block autopilot movement
        return true;
        };

    // target delta
    int dx = tx - head.x;
    int dy = ty - head.y;

    // possible moves (4 directions)
    std::vector<std::pair<int, int>> moves = {
        { sgn(dx), 0 },
        { 0, sgn(dy) },
        { -sgn(dx), 0 },
        { 0, -sgn(dy) }
    };

    // Ensure we evaluate all 4 unique directions
    std::vector<std::pair<int, int>> allDirs = { {1,0},{-1,0},{0,1},{0,-1} };

    // Exclude reverse (can't go directly backwards)
    std::pair<int, int> reverseDir = { -gDirX, -gDirY };

    // Candidate selection: prefer moves that reduce Manhattan distance to target and are safe
    bool found = false;
    int bestDx = gDirX, bestDy = gDirY;
    int bestScore = std::numeric_limits<int>::max();

    for (auto mv : allDirs) {
        if (mv == reverseDir) continue;
        int nx = head.x + mv.first;
        int ny = head.y + mv.second;
        if (!isSafe(nx, ny)) continue;

        // compute Manhattan distance after move (use wrapped coordinates)
        auto p = applyWrap(nx, ny);
        int dist = std::abs(tx - p.first) + std::abs(ty - p.second);

        if (dist < bestScore) {
            bestScore = dist;
            bestDx = mv.first;
            bestDy = mv.second;
            found = true;
        }
    }

    if (found) {
        gPendX = bestDx;
        gPendY = bestDy;
    }
    else {
        // no safe move found: keep current direction (so we at least don't reverse)
        gPendX = gDirX;
        gPendY = gDirY;
    }
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
    ResetSnake();
    PlaceFood();

    gScore = 0;
    gPaused = false;
    gOver = false;

    gMoveInterval = 0.12f;
    gAcc = 0.f;
    gEvents = 0;

    gLevel = 1;
    gEatenInLevel = 0;

    gWrapOn = WRAP_DEFAULT;

    // Xây obstacle theo level 1 (không có chướng ngại)
    BuildLevelObstacles(gLevel);

    // Whirlwind & Ghost
    gTime = 0.f;
    gWhirlwinds.clear();
    gGhostMode = false;
    gGhostRemaining = 0.f;
    gWhirlwindRespawnTimer = 0.f;
    SpawnWhirlwinds(1);

    // AutoPilot & Gate
    gAutoPilot = false;
    gLevelGatePos.x = BOARD_W - 3;
    gLevelGatePos.y = BOARD_H / 2;
}


void Game_Reset() {
    ResetSnake();
    PlaceFood();

    gScore = 0;
    gPaused = false;
    gOver = false;

    gMoveInterval = 0.12f;
    gAcc = 0.f;
    gEvents = 0;

    gLevel = 1;
    gEatenInLevel = 0;

    gWrapOn = WRAP_DEFAULT;

    BuildLevelObstacles(gLevel);

    gTime = 0.f;
    gWhirlwinds.clear();
    gGhostMode = false;
    gGhostRemaining = 0.f;
    gWhirlwindRespawnTimer = 0.f;
    SpawnWhirlwinds(1);

    gAutoPilot = false;
    gLevelGatePos.x = BOARD_W - 3;
    gLevelGatePos.y = BOARD_H / 2;
}


void Game_TogglePause() { if (!gOver) gPaused = !gPaused; }
void Game_SetPaused(bool v) { if (!gOver) gPaused = v; }
void Game_RestartIfOver() { if (gOver) Game_Reset(); }

void Game_OnKeyPressed(int key) {
    if (gAutoPilot) return;  // đang AutoPilot thì bỏ input người chơi
    // Giá trị enum sf::Keyboard (SFML 2.6.x)
    const int K_W = 22, K_A = 0, K_S = 18, K_D = 3, K_UP = 73, K_LEFT = 71, K_DOWN = 74, K_RIGHT = 72;
    if (key == K_W || key == K_UP) { if (gDirY != +1 || gSnake.size() <= 1) { gPendX = 0;  gPendY = -1; } }
    if (key == K_S || key == K_DOWN) { if (gDirY != -1 || gSnake.size() <= 1) { gPendX = 0;  gPendY = +1; } }
    if (key == K_A || key == K_LEFT) { if (gDirX != +1 || gSnake.size() <= 1) { gPendX = -1; gPendY = 0; } }
    if (key == K_D || key == K_RIGHT) { if (gDirX != -1 || gSnake.size() <= 1) { gPendX = +1; gPendY = 0; } }
}

void Game_Update(float dt) {
    if (gPaused || gOver) return;

    // Cập nhật thời gian & whirlwind
    gTime += dt;
    UpdateWhirlwinds(dt);

    // Đếm ngược Ghost Mode (trừ khi đang AutoPilot)
    if (gGhostMode && !gAutoPilot) {
        gGhostRemaining -= dt;
        if (gGhostRemaining <= 0.f) {
            gGhostMode = false;
            gGhostRemaining = 0.f;
        }
    }

    gAcc += dt;
    while (gAcc >= gMoveInterval) {
        gAcc -= gMoveInterval;

        // Nếu AutoPilot thì chọn hướng tự động
        if (gAutoPilot) {
            UpdateAutoPilot();
        }

        // Áp hướng pending
        gDirX = gPendX;
        gDirY = gPendY;

        Cell head = gSnake.front();
        Cell next{ head.x + gDirX, head.y + gDirY };

        bool hitWall = false;

        // WRAP
        if (gWrapOn) {
            if (next.x < 0)         next.x = BOARD_W - 1;
            if (next.x >= BOARD_W) next.x = 0;
            if (next.y < 0)         next.y = BOARD_H - 1;
            if (next.y >= BOARD_H) next.y = 0;
        }
        else {
            if (next.x < 0 || next.x >= BOARD_W ||
                next.y < 0 || next.y >= BOARD_H) {
                hitWall = true;
            }
        }

        // Nếu đang AutoPilot & bước tiếp theo là cổng -> qua màn luôn
        if (gAutoPilot &&
            next.x == gLevelGatePos.x &&
            next.y == gLevelGatePos.y) {

            gSnake.push_front(next);
            if (!gSnake.empty()) gSnake.pop_back();

            StartNextLevel();
            return; // kết thúc frame, sang level mới
        }

        bool hitObs = OccupiedByObstacle(next.x, next.y);
        bool hitSelf = false;
        for (const auto& c : gSnake) {
            if (c.x == next.x && c.y == next.y) {
                hitSelf = true;
                break;
            }
        }

        // Kiểm tra va vào Whirlwind -> bật Ghost Mode
        if (!gGhostMode) {
            for (std::size_t i = 0; i < gWhirlwinds.size(); ++i) {
                const Cell& wpos = gWhirlwinds[i].pos;
                if ((wpos.x == next.x && wpos.y == next.y) ||
                    (wpos.x == head.x && wpos.y == head.y)) {

                    gGhostMode = true;
                    gGhostRemaining = 3.0f;

                    RemoveWhirlwindAt(wpos.x, wpos.y);

                    // Bỏ qua va chạm trong bước này
                    hitWall = hitObs = hitSelf = false;
                    break;
                }
            }
        }

        // Xử lý chết / sống
        if (!gGhostMode) {
            if (hitWall || hitObs || hitSelf) {
                gOver = true;
                gEvents |= (1 << 1); // DIE
                break;
            }
        }
        else {
            // Ghost mode: bỏ qua obstacle & tự va,
            // nhưng vẫn giữ rắn trong map nếu không wrap
            if (!gWrapOn && hitWall) {
                if (next.x < 0)              next.x = 0;
                else if (next.x >= BOARD_W)  next.x = BOARD_W - 1;
                if (next.y < 0)              next.y = 0;
                else if (next.y >= BOARD_H)  next.y = BOARD_H - 1;
            }
        }

        if (gOver) break;

        gSnake.push_front(next);

        // Ăn mồi?
        if (next.x == gFoodX && next.y == gFoodY) {
            gScore += 1;
            if (gScore > gHighScore) gHighScore = gScore;
            gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.006f);
            PlaceFood();

            gEatenInLevel++;
            if (gEatenInLevel >= LEVEL_STEP && !gAutoPilot) {
                // Đã đạt số mồi để qua màn -> bật AutoPilot
                gEatenInLevel = 0;
                StartAutoPilotToGate();
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

bool Game_IsGhost() { return gGhostMode; }

std::size_t Game_WhirlwindCount() { return gWhirlwinds.size(); }
Cell        Game_Whirlwind(std::size_t i) { return gWhirlwinds[i].pos; }

bool Game_IsAutoPilot() { return gAutoPilot; }
Cell Game_PortalPos() { return gLevelGatePos; }

int Game_ConsumeEvents() { int e = gEvents; gEvents = 0; return e; }

// Wrap getters/toggles
void Game_ToggleWrap() { if (!gOver) gWrapOn = !gWrapOn; }
bool Game_WrapOn() { return gWrapOn; }

// ===================== SAVE / LOAD GAME =====================

// Lưu game dựa trên tên người chơi (Profile) hiện tại
bool Game_SaveGame()
{
    std::string filename = GetSaveFileName(gCurrentProfileName);
    std::ofstream out(filename);
    if (!out) return false;

    // 1. Thông tin chung
    out << gScore << ' '
        << gHighScore << ' '
        << gLevel << ' '
        << gEatenInLevel << ' '
        << gMoveInterval << ' '
        << gDirX << ' ' << gDirY << ' '
        << gPendX << ' ' << gPendY << ' '
        << (int)gWrapOn << '\n';

    // 2. Food
    out << gFoodX << ' ' << gFoodY << '\n';

    // 3. Thân rắn
    out << gSnake.size() << '\n';
    for (const auto& c : gSnake) {
        out << c.x << ' ' << c.y << '\n';
    }

    // 4. Obstacles
    out << gObstacles.size() << '\n';
    for (const auto& o : gObstacles) {
        out << o.x << ' ' << o.y << '\n';
    }

    return true;
}

// Tải game dựa trên tên người chơi (Profile) hiện tại
bool Game_LoadGame()
{
    std::string filename = GetSaveFileName(gCurrentProfileName);
    std::ifstream in(filename);
    if (!in) return false;

    int score, hi, level, eatenLv;
    float moveInt;
    int dirX, dirY, pendX, pendY;
    int wrapOn;

    // 1. Thông tin chung
    if (!(in >> score >> hi >> level >> eatenLv >> moveInt
        >> dirX >> dirY >> pendX >> pendY >> wrapOn))
        return false;

    int foodX, foodY;
    // 2. Food
    if (!(in >> foodX >> foodY)) return false;

    // 3. Thân rắn
    size_t snakeLen;
    if (!(in >> snakeLen)) return false;
    std::deque<Cell> snakeTmp;
    for (size_t i = 0; i < snakeLen; ++i) {
        Cell c;
        if (!(in >> c.x >> c.y)) return false;
        snakeTmp.push_back(c);
    }

    // 4. Obstacles
    size_t obsLen;
    if (!(in >> obsLen)) return false;
    std::vector<Cell> obsTmp;
    obsTmp.reserve(obsLen);
    for (size_t i = 0; i < obsLen; ++i) {
        Cell o;
        if (!(in >> o.x >> o.y)) return false;
        obsTmp.push_back(o);
    }

    // Assign loaded state
    gScore = score;
    gHighScore = hi;
    gLevel = level;
    gEatenInLevel = eatenLv;
    gMoveInterval = moveInt;
    gDirX = dirX; gDirY = dirY;
    gPendX = pendX; gPendY = pendY;
    gWrapOn = (wrapOn != 0);
    gFoodX = foodX; gFoodY = foodY;
    gSnake = std::move(snakeTmp);
    gObstacles = std::move(obsTmp);

    // Reset runtime flags
    gPaused = false;
    gOver = false;
    gAcc = 0.f;
    gEvents = 0;

    return true;
}

// =========================================================
// ===== PROFILE HELPERS =====
// =========================================================
void Game_SetProfileName(const std::string& name) {
    gCurrentProfileName = name;
}

std::string Game_GetProfileName() {
    return gCurrentProfileName;
}

std::vector<std::string> Game_GetProfileList() {
    std::vector<std::string> list;
    std::ifstream in(PROFILE_LIST_FILE);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) list.push_back(line);
    }
    return list;
}

bool Game_CreateProfile(const std::string& name) {
    if (name.empty()) return false;
    auto list = Game_GetProfileList();
    for (const auto& s : list) if (s == name) return false;
    std::ofstream out(PROFILE_LIST_FILE, std::ios::app);
    out << name << "\n";
    return true;
}

std::string Game_GetProfileInfo(const std::string& name) {
    std::string filename = GetSaveFileName(name);
    std::ifstream in(filename);
    if (!in) return "New Player - No Data";
    int score, hi, level;
    if (in >> score >> hi >> level) {
        return "Level: " + std::to_string(level) + " | Score: " + std::to_string(score);
    }
    return "Error reading data";
}