#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>

// Định nghĩa MenuHit ngay tại đây để các file khác dùng chung
enum MenuHit {
    MH_None = 0,
    MH_Resume,
    MH_Restart,
    MH_Save,
    MH_Load,
    MH_Exit
};

// Khởi tạo (gọi trong main để load font, texture cho UI)
void Ui_Init(sf::RenderWindow& window);

// ===== Pause Menu =====
void    Gfx_MenuUpdateHover(const sf::Vector2f& mouse);
MenuHit Gfx_MenuHitTest(const sf::Vector2f& mouse);
void    Gfx_DrawPauseMenu(sf::RenderWindow& window);

// ===== Snake Skin System =====
void        Gfx_SetSkin(int id);
int         Gfx_CurrentSkin();
const char* Gfx_SkinName();

// ===== Skin Picker Menu (F1) =====
void Gfx_SkinMenuToggle();
bool Gfx_SkinMenuOn();
void Gfx_SkinMenuOnEvent(const sf::Event& e);
void Gfx_SkinMenuDraw(sf::RenderWindow& window);

// ==== Main Menu & Options ====
void Gfx_MenuLayout(int winW, int winH);
void Gfx_DrawMainMenu(sf::RenderWindow& window, int previewSkin);
void Gfx_DrawOptions(sf::RenderWindow& window, int previewSkin);
int  Gfx_OptionsHitTest(sf::Vector2f m); // Xử lý nút Back trong Options

// Getter cho rect nút bấm Main Menu (để xử lý click trong main.cpp)
const sf::FloatRect& Gfx_BtnPlay();
const sf::FloatRect& Gfx_BtnOptions();
const sf::FloatRect& Gfx_BtnQuit();

// ==========================================
// === PROFILE MENUS (Profile System) ===
// ==========================================
void Gfx_DrawProfileMenu(sf::RenderWindow& window);
int  Gfx_ProfileMenuHitTest(sf::Vector2f m); // 1=New, 2=Load, 3=Back

void Gfx_DrawNameInput(sf::RenderWindow& window, const std::string& currentInput);

void Gfx_DrawProfileSelect(sf::RenderWindow& window, const std::vector<std::string>& list, int indexSelected);
int  Gfx_ProfileSelectHitTest(sf::Vector2f m, int listSize);

// ==========================================
// === GAME OVER SCREEN ===
// ==========================================
void Gfx_DrawGameOverOverlay(sf::RenderWindow& window);
int  Gfx_GameOverHitTest(sf::Vector2f m); // 1=Restart, 2=Exit

// ----------------- Audio control -----------------
void Ui_SetAudioEnabled(bool enabled);
bool Ui_AudioEnabled();