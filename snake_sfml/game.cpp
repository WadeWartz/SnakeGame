#include <SFML/Graphics.hpp>
#include "game.h"
#include "config.h"
#include <deque>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <iostream>

// ===== CẤU HÌNH LOGIC =====
const int FOOD_TO_PASS = 10;

// ===== RNG =====
static std::mt19937& Rng() { static std::mt19937 g{ std::random_device{}() }; return g; }
static int RandInt(int lo, int hi) { std::uniform_int_distribution<int> d(lo, hi); return d(Rng()); }

// ===== Trạng thái =====
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

// Level & Obstacle
static int gLevel = 1;
static int gEatenInLevel = 0;
static std::vector<Cell> gObstacles;

// Whirlwind
//struct Whirlwind { Cell pos; };
struct Whirlwind {
    Cell pos;
    float spawnTime;  // thời điểm spawn
    float lifeTime;   // thời gian tồn tại (3 giây)
};

//Thêm bộ đếm thời gian cho việc tái sinh Whirlwind
static float gWhirlwindRespawnTimer = 0.f;   // Thời gian chờ để spawn lại
static const float WHIRL_DESPAWN_TIME = 3.0f; // tồn tại 3 giây
static const float WHIRL_RESPAWN_DELAY = 2.0f; // xuất hiện lại sau 2 giây

static std::vector<Whirlwind> gWhirlwinds;
static float gTime = 0.f;
static bool gGhostMode = false;
static float gGhostRemaining = 0.f;

// ===== AUTO PILOT & PORTAL (MỚI) =====
static bool gAutoPilot = false;       // Đang trong chế độ tự đi?
static Cell gPortalPos = { -1, -1 };  // Vị trí cổng

// High score path
static const char* SAVE_PATH = "save_highscore.txt";

// ====== HÀM TIỆN ÍCH ======
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
//static void RemoveWhirlwindAt(int x, int y) {
//    for (auto it = gWhirlwinds.begin(); it != gWhirlwinds.end(); ++it) {
//        if (it->pos.x == x && it->pos.y == y) { gWhirlwinds.erase(it); return; }
//    }
//}

static void RemoveWhirlwindAt(int x, int y) {
    gWhirlwinds.erase(
        std::remove_if(gWhirlwinds.begin(), gWhirlwinds.end(),
            [&](auto& w) { return w.pos.x == x && w.pos.y == y; }),
        gWhirlwinds.end()
    );
}

// Spawn các vật thể
static void SpawnObstacles(int count) {
    for (int k = 0; k < count; ++k) {
        int ox, oy, guard = 0;
        do {
            ox = RandInt(0, BOARD_W - 1); oy = RandInt(0, BOARD_H - 1);
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



bool Game_IsGhost() { return gGhostMode; }

static void UpdateWhirlwinds(float dt)
{
    // --- 1) Xóa whirlwind đã quá 3 giây ---
    bool anyRemoved = false;

    gWhirlwinds.erase(
        std::remove_if(
            gWhirlwinds.begin(),
            gWhirlwinds.end(),
            [&](const Whirlwind& w) {
                bool dead = (gTime - w.spawnTime) >= w.lifeTime;
                if (dead) anyRemoved = true;
                return dead;
            }
        ),
        gWhirlwinds.end()
    );

    // --- 2) Nếu tất cả whirlwind đã biến mất, bắt đầu đếm 2 giây ---
    if (gWhirlwinds.empty()) {
        gWhirlwindRespawnTimer += dt;

        // --- 3) Spawn lại sau 2 giây ---
        if (gWhirlwindRespawnTimer >= WHIRL_RESPAWN_DELAY) {
            SpawnWhirlwinds(1);      // bạn có thể tăng số lượng tùy ý
            gWhirlwindRespawnTimer = 0.f;
        }
    }
    else {
        // Nếu vẫn đang có whirlwind → reset timer để tránh spawn trùng
        gWhirlwindRespawnTimer = 0.f;
    }
}


static void PlaceFood() {
    // Nếu đang Auto Pilot thì không tạo mồi nữa, giấu mồi đi
    if (gAutoPilot) { gFoodX = -10; gFoodY = -10; return; }

    int guard = 0;
    while (guard++ < 2000) {
        gFoodX = RandInt(0, BOARD_W - 1); gFoodY = RandInt(0, BOARD_H - 1);
        if (OccupiedBySnake(gFoodX, gFoodY)) continue;
        if (OccupiedByObstacle(gFoodX, gFoodY)) continue;
        if (OccupiedByWhirlwind(gFoodX, gFoodY)) continue;
        break;
    }
}

// ===== LOGIC CHÍNH =====
static void ResetSnake() {
    gSnake.clear();
    int sx = BOARD_W / 2, sy = BOARD_H / 2;

    // Độ dài ban đầu là 8
    for (int i = 0; i < 8; ++i) {
        gSnake.push_back({ sx - i, sy });
    }

    gDirX = +1; gDirY = 0; gPendX = +1; gPendY = 0;
}

static void LoadLevel(int level) {
    gLevel = level;
    gEatenInLevel = 0;
    gAutoPilot = false;     // Tắt chế độ tự lái
    gPortalPos = { -1, -1 }; // Giấu cổng đi

    gObstacles.clear();
    gWhirlwinds.clear();
    ResetSnake();
    gMoveInterval = 0.12f;

    if (gLevel == 2) { SpawnObstacles(5); gMoveInterval = 0.11f; }
    else if (gLevel == 3) { SpawnObstacles(8); SpawnWhirlwinds(4); gMoveInterval = 0.10f; }

    PlaceFood();
}

// ===== XỬ LÝ AUTO PILOT (TỰ TÌM ĐƯỜNG) =====
static void UpdateAutoPilot() {
    Cell head = gSnake.front();
    int tx = gPortalPos.x;
    int ty = gPortalPos.y;

    // Logic đơn giản: Ưu tiên đi theo trục nào xa hơn
    // và đảm bảo không quay ngược đầu 180 độ

    int dx = tx - head.x;
    int dy = ty - head.y;

    int moveX = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int moveY = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

    // Thử đi theo trục X trước nếu khoảng cách X lớn hơn
    bool tryX = (std::abs(dx) >= std::abs(dy));

    // Hàm kiểm tra xem hướng đi có hợp lệ (không quay đầu 180 độ)
    auto isValidDir = [&](int mx, int my) {
        if (mx == 0 && my == 0) return false;
        if (mx == -gDirX && my == -gDirY) return false; // Không quay đầu
        return true;
        };

    int nextX = gDirX, nextY = gDirY; // Mặc định giữ hướng cũ

    if (tryX && moveX != 0 && isValidDir(moveX, 0)) {
        nextX = moveX; nextY = 0;
    }
    else if (moveY != 0 && isValidDir(0, moveY)) {
        nextX = 0; nextY = moveY;
    }
    else if (moveX != 0 && isValidDir(moveX, 0)) {
        // Nếu trục Y không đi được thì thử lại trục X
        nextX = moveX; nextY = 0;
    }

    // Gán hướng đi mới
    gPendX = nextX; gPendY = nextY;
}

// ===== API =====
void Game_Init() { gScore = 0; gPaused = false; gOver = false; gAcc = 0.f; gEvents = 0; LoadLevel(1); }
void Game_Reset() { Game_Init(); }
void Game_TogglePause() { if (!gOver) gPaused = !gPaused; }
void Game_SetPaused(bool v) { if (!gOver) gPaused = v; }
void Game_RestartIfOver() { if (gOver) Game_Reset(); }

void Game_OnKeyPressed(int key) {
    if (gAutoPilot) return; // Đang tự đi thì không nhận phím

    const int K_W = 22, K_UP = 73, K_S = 18, K_DOWN = 74, K_A = 0, K_LEFT = 71, K_D = 3, K_RIGHT = 72;
    if ((key == K_W || key == K_UP) && (gDirY != +1)) { gPendX = 0; gPendY = -1; }
    if ((key == K_S || key == K_DOWN) && (gDirY != -1)) { gPendX = 0; gPendY = +1; }
    if ((key == K_A || key == K_LEFT) && (gDirX != +1)) { gPendX = -1; gPendY = 0; }
    if ((key == K_D || key == K_RIGHT) && (gDirX != -1)) { gPendX = +1; gPendY = 0; }
}

void Game_Update(float dt) {
    if (gPaused || gOver) return;
    gTime += dt;
    UpdateWhirlwinds(dt);


    if (gGhostMode) {
        gGhostRemaining -= dt;
        if (gGhostRemaining <= 0.f) {
            gGhostMode = false;
            gGhostRemaining = 0.f;
        }
    }

    gAcc += dt;

    while (gAcc >= gMoveInterval) {
        gAcc -= gMoveInterval;

        // Nếu đang chế độ tự lái, tính toán hướng đi
        if (gAutoPilot) {
            UpdateAutoPilot();
        }

        gDirX = gPendX; gDirY = gPendY;
        Cell head = gSnake.front();
        Cell next{ head.x + gDirX, head.y + gDirY };

        // --- XỬ LÝ VA CHẠM ---
        bool hitWall = (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H);
        bool hitObs = OccupiedByObstacle(next.x, next.y);
        bool hitSelf = false;
        for (const auto& c : gSnake) if (c.x == next.x && c.y == next.y) hitSelf = true;

        if (gAutoPilot) {
            // == LOGIC KHI TỰ ĐI ==
            // Rắn thành "siêu nhân": đi xuyên tường (wrap) hoặc xuyên chướng ngại vật để về đích cho đẹp
            if (hitObs || hitSelf) { /* Bỏ qua va chạm khi đang cutscene */ }

            // Nếu đụng tường thì wrap (đi vòng sang bên kia) để tìm đường tiếp
            if (next.x < 0) next.x = BOARD_W - 1;
            else if (next.x >= BOARD_W) next.x = 0;
            else if (next.y < 0) next.y = BOARD_H - 1;
            else if (next.y >= BOARD_H) next.y = 0;

            // KIỂM TRA VỀ ĐÍCH (CỔNG)
            if (next.x == gPortalPos.x && next.y == gPortalPos.y) {
                // Qua màn!
                gLevel++;
                if (gLevel > 3) gLevel = 1;
                LoadLevel(gLevel);
                return; // Kết thúc update frame này
            }
        }
        else {
            // == LOGIC CHƠI BÌNH THƯỜNG ==
            /*if (hitWall || hitObs || hitSelf) { gOver = true; gEvents |= (1 << 1); break; }*/

            if (!gGhostMode) {
                // Chết bình thường
                if (hitWall || hitObs || hitSelf) {
                    gOver = true;
                    gEvents |= (1 << 1);
                    break;
                }
            }
            else {
                // Ghost mode → bỏ qua va chạm
                hitWall = hitObs = hitSelf = false;

                // **VẪN GIỚI HẠN TƯỜNG**
                if (hitWall) {
                    // Nếu rắn chạm tường → đặt vị trí vào trong map
                    if (next.x < 0) next.x = 0;
                    else if (next.x >= BOARD_W) next.x = BOARD_W - 1;
                    if (next.y < 0) next.y = 0;
                    else if (next.y >= BOARD_H) next.y = BOARD_H - 1;
                }
            }

            // Duỗi từng whirlwind trong danh sách
            for (auto& w : gWhirlwinds) {
                // Kiểm tra xem đầu rắn sẽ đi vào whirlwind này không
                if (w.pos.x == next.x && w.pos.y == next.y) {
                    // Kích hoạt ghost mode
                    gGhostMode = true;
                    gGhostRemaining = 3.0f;

                    // Xoá whirlwind vừa chạm
                    RemoveWhirlwindAt(w.pos.x, w.pos.y);

                    // Chỉ kích hoạt 1 whirlwind mỗi bước
                    break;
                }
            }

            // Whirlwind logic
            //if (OccupiedByWhirlwind(next.x, next.y)) {
            //    RemoveWhirlwindAt(next.x, next.y);
            //    int oldDx = gDirX, oldDy = gDirY;
            //    gDirX = -oldDy; gDirY = oldDx; // Xoay 90 độ
            //    gPendX = gDirX; gPendY = gDirY;
            //    next = { head.x + gDirX, head.y + gDirY };
            //    if (next.x < 0 || next.x >= BOARD_W || next.y < 0 || next.y >= BOARD_H) {
            //        gOver = true; gEvents |= (1 << 1); break;
            //    }
            //}

     
        }

        gSnake.push_front(next);

        // Ăn mồi (chỉ khi không phải Auto Pilot)
        if (!gAutoPilot && next.x == gFoodX && next.y == gFoodY) {
            gScore++;
            gEvents |= (1 << 0); // Sound EAT
            if (gScore > gHighScore) gHighScore = gScore;

            // Lấy độ dài chuỗi mục tiêu
            std::string targetStr = MSSV_FULL;

            // KIỂM TRA QUA MÀN: Nếu độ dài rắn >= độ dài chuỗi số
            if (gSnake.size() >= targetStr.length()) {
                // KÍCH HOẠT CHẾ ĐỘ TỰ ĐI (AUTO PILOT) QUA CỔNG
                gAutoPilot = true;
                gPortalPos = { BOARD_W - 1, BOARD_H / 2 };
                gFoodX = -10; gFoodY = -10; // Giấu mồi

                // Yêu cầu: "Giảm tốc độ rắn xuống 1 tí" khi hoàn thành
                // Tăng interval lên -> rắn đi chậm lại
                gMoveInterval += 0.03f;
            }
            else {
                PlaceFood();
                // Bình thường thì ăn xong rắn chạy nhanh hơn 1 chút
                gMoveInterval = std::max(gMinInterval, gMoveInterval - 0.002f);
            }

            // LƯU Ý QUAN TRỌNG: 
            // Không gọi gSnake.pop_back() ở nhánh else bên dưới
            // vì ta muốn rắn dài ra (thêm số) sau khi ăn.
        }
        else {
            // Nếu không ăn mồi thì xóa đuôi để giữ nguyên độ dài
            gSnake.pop_back();
        }
    }
}

// Getters & Load/Save (giữ nguyên)
void Game_LoadHighScore() { std::ifstream fin(SAVE_PATH); if (fin) fin >> gHighScore; }
void Game_SaveHighScore() { static int l = -1; if (gHighScore != l) { std::ofstream f(SAVE_PATH); if (f) f << gHighScore; l = gHighScore; } }
int Game_HighScore() { return gHighScore; }
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
std::size_t Game_WhirlwindCount() { return gWhirlwinds.size(); }
Cell Game_Whirlwind(std::size_t i) { return gWhirlwinds[i].pos; }
int Game_ConsumeEvents() { int e = gEvents; gEvents = 0; return e; }

// Getter cho cổng (để bên gfx vẽ)
bool Game_IsAutoPilot() { return gAutoPilot; }
Cell Game_PortalPos() { return gPortalPos; }
