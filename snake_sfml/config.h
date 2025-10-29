#pragma once

constexpr int TILE = 32;
constexpr int BORDER = 1;
constexpr int BOARD_W = 30;
constexpr int BOARD_H = 20;
constexpr int LEVEL_STEP = 5;    // ăn 5 mồi -> lên 1 level
constexpr int OBSTACLE_PER_LV = 2;    // mỗi level sinh thêm 2 chướng ngại
// --- Portal & Wrap ---
constexpr int  PORTAL_MIN_LEVEL = 2;   // từ Level này trở đi mới có Portal
constexpr bool WRAP_DEFAULT = false; // mặc định có/không xuyên tường


//Một ô lưới
struct Cell { int x, y; };

// Chuoi MSSV
#define MSSV_STRING "22123456"
