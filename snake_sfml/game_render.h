#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

// Khởi tạo
bool Gfx_Init(sf::RenderWindow& window);

// Vẽ gameplay chính
void Gfx_DrawFrame(sf::RenderWindow& window);

// Pause menu
enum MenuHit { MH_None = 0, MH_Resume, MH_Restart, MH_Save, MH_Load, MH_Exit };
void    Gfx_MenuUpdateHover(const sf::Vector2f& mouse);
MenuHit Gfx_MenuHitTest(const sf::Vector2f& mouse);

// Snake skin
void        Gfx_SetSkin(int id);
int         Gfx_CurrentSkin();
const char* Gfx_SkinName();
