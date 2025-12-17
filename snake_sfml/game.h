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

// === Input ===
void Game_OnKeyPressed(int sfmlKeyCode);

// === Update ===
void Game_Update(float dt);

// Save / Load
bool Game_SaveGame();
bool Game_LoadGame();

// === Getters ===
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

bool        Game_IsGhost();
std::size_t Game_WhirlwindCount();
Cell        Game_Whirlwind(std::size_t i);

// Auto Pilot & Gate
bool        Game_IsAutoPilot();
Cell        Game_PortalPos();
bool        Game_IsExiting(); //Kiểm tra có đang chui cổng không

int         Game_ConsumeEvents();

// === High Score ===
void Game_LoadHighScore();
void Game_SaveHighScore();

// === Wrap ===
void Game_ToggleWrap();
bool Game_WrapOn();

// === PROFILE SYSTEM ===
void                     Game_SetProfileName(const std::string& name);
std::string              Game_GetProfileName();
bool                     Game_CreateProfile(const std::string& name);
std::vector<std::string> Game_GetProfileList();
std::string              Game_GetProfileInfo(const std::string& name);