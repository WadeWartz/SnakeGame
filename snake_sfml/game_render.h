#pragma once
#include "ui_gfx.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

// Khởi tạo
bool Gfx_Init(sf::RenderWindow& window);

// Vẽ gameplay chính
void Gfx_DrawFrame(sf::RenderWindow& window);

// Pause menu
void    Gfx_MenuUpdateHover(const sf::Vector2f& mouse);
MenuHit Gfx_MenuHitTest(const sf::Vector2f& mouse);

// Snake skin
void        Gfx_SetSkin(int id);
int         Gfx_CurrentSkin();
const char* Gfx_SkinName();
