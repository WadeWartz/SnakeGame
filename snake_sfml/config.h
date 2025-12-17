#pragma once

constexpr int TILE = 32;
constexpr int BORDER = 1;
constexpr int BOARD_W = 30;
constexpr int BOARD_H = 20;

// Số này đặt lớn để không ảnh hưởng logic thắng (ta dùng độ dài chuỗi để tính thắng)
constexpr int LEVEL_STEP = 9999;
constexpr int OBSTACLE_PER_LV = 2;

// --- Portal & Wrap ---
constexpr int  PORTAL_MIN_LEVEL = 2;
constexpr bool WRAP_DEFAULT = false;

// --- CẤU HÌNH ĐỘ DÀI ---
// Độ dài ban đầu của rắn là 5
constexpr int INITIAL_SNAKE_LEN = 6;

// Đích đến (Mã số sinh viên chuẩn): Chỉ cần đạt độ dài này là qua màn
#define MSSV_FULL "24127081"

// Dùng macro này để vẽ số
#define MSSV_STRING MSSV_FULL

struct Cell { int x, y; };