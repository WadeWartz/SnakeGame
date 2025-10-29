#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

// Khởi tạo tài nguyên đồ hoạ/âm thanh
bool Gfx_Init(sf::RenderWindow& window);

// Vẽ 1 frame
void Gfx_DrawFrame(sf::RenderWindow& window);

// ===== Pause Menu (nếu bạn đang dùng) =====
enum MenuHit { MH_None = 0, MH_Resume, MH_Restart, MH_Exit };
void    Gfx_MenuUpdateHover(const sf::Vector2f& mouse);
MenuHit Gfx_MenuHitTest(const sf::Vector2f& mouse);

// ===== Snake Skin (đã có phím 1..6) =====
void        Gfx_SetSkin(int id);     // 0..5
int         Gfx_CurrentSkin();
const char* Gfx_SkinName();

// ===== Skin Picker Menu (F1) =====
void Gfx_SkinMenuToggle();                         // Bật/tắt menu
bool Gfx_SkinMenuOn();                             // Menu đang mở?
void Gfx_SkinMenuOnEvent(const sf::Event& e);      // Gửi event cho menu (khi mở)
void Gfx_SkinMenuDraw(sf::RenderWindow& window);   // Vẽ menu (gọi cuối frame)

// ==== Main Menu & Options (chọn skin trước khi chơi) ====
void Gfx_MenuLayout(int winW, int winH);
void Gfx_DrawMainMenu(sf::RenderWindow& window, int previewSkin /*0..5*/);
void Gfx_DrawOptions(sf::RenderWindow& window, int previewSkin /*0..5*/);

// Trả về rect nút để hit-test đơn giản (chuột)
const sf::FloatRect& Gfx_BtnPlay();
const sf::FloatRect& Gfx_BtnOptions();
const sf::FloatRect& Gfx_BtnQuit();
