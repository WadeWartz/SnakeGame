#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>

// Forward-declare the shared enum to avoid a second definition.
// The actual enum is defined in game_render.h:
enum MenuHit;

// Khởi tạo
bool Gfx_Init(sf::RenderWindow& window);

// Vẽ 1 frame game
void Gfx_DrawFrame(sf::RenderWindow& window);

// ===== Pause Menu =====
void    Gfx_MenuUpdateHover(const sf::Vector2f& mouse);
MenuHit Gfx_MenuHitTest(const sf::Vector2f& mouse);

// ===== Snake Skin =====
void        Gfx_SetSkin(int id);
int         Gfx_CurrentSkin();
const char* Gfx_SkinName();

// ===== Skin Picker Menu =====
void Gfx_SkinMenuToggle();
bool Gfx_SkinMenuOn();
void Gfx_SkinMenuOnEvent(const sf::Event& e);
void Gfx_SkinMenuDraw(sf::RenderWindow& window);

// ==== Main Menu & Options ====
void Gfx_MenuLayout(int winW, int winH);
void Gfx_DrawMainMenu(sf::RenderWindow& window, int previewSkin);
void Gfx_DrawOptions(sf::RenderWindow& window, int previewSkin);

const sf::FloatRect& Gfx_BtnPlay();
const sf::FloatRect& Gfx_BtnOptions();
const sf::FloatRect& Gfx_BtnQuit();

// ==========================================
// === PROFILE MENUS (MỚI THÊM) ===
// ==========================================

void Gfx_DrawProfileMenu(sf::RenderWindow& window);
int Gfx_ProfileMenuHitTest(sf::Vector2f m);
void Gfx_DrawNameInput(sf::RenderWindow& window, const std::string& currentInput);
void Gfx_DrawProfileSelect(sf::RenderWindow& window, const std::vector<std::string>& list, int indexSelected);
int Gfx_ProfileSelectHitTest(sf::Vector2f m, int listSize);
int Gfx_OptionsHitTest(sf::Vector2f m);

// ----------------- Audio control (added) -----------------
void Ui_SetAudioEnabled(bool enabled);
bool Ui_AudioEnabled();

void Ui_Init(sf::RenderWindow& window);

void Gfx_DrawPauseMenu(sf::RenderWindow& window);

// New: Game Over overlay + hit test
void Gfx_DrawGameOverOverlay(sf::RenderWindow& window);
int  Gfx_GameOverHitTest(sf::Vector2f m);