#pragma once
#include "config.h"
#include <cstddef>

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

// Sự kiện 1 lần: bit0=EAT, bit1=DIE
int         Game_ConsumeEvents();

// === High Score (file) ===
void Game_LoadHighScore();
void Game_SaveHighScore();

// === Wrap & Portal ===
void Game_ToggleWrap();
bool Game_WrapOn();

bool Game_PortalsActive();
Cell Game_PortalA();
Cell Game_PortalB();
