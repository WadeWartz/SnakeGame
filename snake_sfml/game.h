#pragma once
#include "config.h"
#include <cstddef>
#include <string>
#include <vector>
// === Vòng đời & trạng thái ===
void Game_Init();
void Game_Reset();
void Game_TogglePause();
void Game_SetPaused(bool v);
void Game_RestartIfOver();

// === Input hướng (nhận mã phím của SFML dưới dạng int) ===
void Game_OnKeyPressed(int sfmlKeyCode);

// === Cập nhật theo thời gian (dt = giây) ===
void Game_Update(float dt);

// Save / Load toàn bộ trạng thái game (vị trí rắn, level, tốc độ, obstacle...)
bool Game_SaveGame();   // nhấn L
bool Game_LoadGame();   // nhấn T


// === Getters dùng cho GFX ===
int         Game_Score();
int         Game_HighScore();
bool        Game_Paused();
bool        Game_Over();
float       Game_MoveInterval();
int         Game_Level();

std::size_t Game_SnakeLen();
Cell        Game_SnakeSeg(std::size_t i);
Cell        Game_Food();

std::size_t Game_ObstacleCount();
Cell        Game_Obstacle(std::size_t i);

bool Game_IsGhost();
std::size_t Game_WhirlwindCount();
Cell Game_Whirlwind(std::size_t i);

// Auto Pilot & Gate (Torii)
bool        Game_IsAutoPilot();
Cell        Game_PortalPos();

// Sự kiện 1 lần: bit0=EAT, bit1=DIE
int         Game_ConsumeEvents();

// === High Score (file) ===
void Game_LoadHighScore();
void Game_SaveHighScore();

// === Wrap ===
void Game_ToggleWrap();
bool Game_WrapOn();

// === PROFILE SYSTEM ===
void Game_SetProfileName(const std::string& name);
std::string Game_GetProfileName();

// Tạo profile mới (trả về false nếu tên đã tồn tại)
bool Game_CreateProfile(const std::string& name);

// Lấy danh sách tất cả profile đã tạo
std::vector<std::string> Game_GetProfileList();

// Lấy thông tin tóm tắt của 1 profile để hiển thị (Level, Score)
// Trả về chuỗi dạng "Level: 5 | Score: 1200"
std::string Game_GetProfileInfo(const std::string& name);
