#include "game_render.h"
#include "config.h"
#include "game.h"
#include "ui_gfx.h"            
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>

// ----------------- GLOBALS (gameplay-only) -----------------
static sf::Font gFont;
static sf::Text gHud, gSeg;

static sf::RectangleShape gHeadRect, gBodyRect;
static sf::CircleShape    gFoodShape;
static sf::RectangleShape gObsRect;

// Background
static sf::Texture gBgTextures[3];
static sf::Sprite  gBgSprites[3];
static bool        gBgLoaded[3] = { false, false, false };

// Tornado
static sf::Texture texWhirlwind;
static bool        texWhirlwindLoaded = false;

// Rock
static sf::Texture gRockTexture;
static sf::Sprite  gRockSprite;
static bool        gRockLoaded = false;

// Torii gate
static sf::Texture texTorii;
static sf::Sprite  sprTorii;
static bool        toriiLoaded = false;

// GameOver bg
static sf::Texture gGameOverBgTexture;
static sf::Sprite  gGameOverBgSprite;
static bool        gGameOverBgLoaded = false;

// Sound
static sf::SoundBuffer gEatBuf;
static sf::Sound gEat;
static sf::SoundBuffer gDieBuf;
static sf::Sound gDie;

// Helpers
static float s_lerp(float a, float b, float t) { return a + (b - a) * t; }
static float s_smooth(float t) { return t * t * (3.f - 2.f * t); }
static inline sf::Vector2f CellToPx(int cx, int cy) {
    return sf::Vector2f((float)(cx + BORDER) * TILE, (float)(cy + BORDER) * TILE);
}

// ----------------- SNAKE SKIN SYSTEM -----------------
static int gSkin = 0;
static sf::Color FromHSV(float h, float s, float v) {
    float c = v * s, x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f)), m = v - c;
    float r = 0, g = 0, b = 0;
    if (0 <= h && h < 60) { r = c; g = x; b = 0; }
    else if (60 <= h && h < 120) { r = x; g = c; b = 0; }
    else if (120 <= h && h < 180) { r = 0; g = c; b = x; }
    else if (180 <= h && h < 240) { r = 0; g = x; b = c; }
    else if (240 <= h && h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    return sf::Color((sf::Uint8)((r + m) * 255), (sf::Uint8)((g + m) * 255), (sf::Uint8)((b + m) * 255));
}
static sf::Color LerpColor(const sf::Color& a, const sf::Color& b, float t) {
    return sf::Color((sf::Uint8)(a.r + (b.r - a.r) * t), (sf::Uint8)(a.g + (b.g - a.g) * t), (sf::Uint8)(a.b + (b.b - a.b) * t));
}
static sf::Color SkinColorFor(std::size_t i, std::size_t len, bool head) {
    switch (gSkin) {
    case 0: return head ? sf::Color(0, 180, 120) : sf::Color(0, 140, 90);
    case 1: { if (head) return sf::Color(0, 255, 255); float t = (std::sin((float)i * 0.5f) + 1.f) * 0.5f; return LerpColor(sf::Color(0, 100, 200), sf::Color(0, 200, 255), t); }
    case 2: { float h = 360.f * ((float)i / (float)(len > 0 ? len : 1)); return FromHSV(h, 0.8f, 1.f); }
    case 3: { return (i % 2 == 0) ? sf::Color(255, 200, 50) : sf::Color(50, 50, 50); }
    case 4: { float t = (float)i / (float)(len > 0 ? len : 1); return LerpColor(sf::Color(255, 50, 0), sf::Color(50, 0, 0), t); }
    case 5: return head ? sf::Color(255, 215, 0) : sf::Color(180, 150, 20);
    default: return sf::Color::White;
    }
}
void Gfx_SetSkin(int id) { gSkin = std::clamp(id, 0, 5); }
int  Gfx_CurrentSkin() { return gSkin; }
const char* Gfx_SkinName() { static const char* N[] = { "Classic","Neon","Rainbow","Stripes","Magma","Gold" }; return N[std::clamp(gSkin, 0, 5)]; }

// ----------------- Draw tornado -----------------
static void DrawTornado(sf::RenderWindow& win, int gx, int gy, float t) {
    if (!texWhirlwindLoaded) return;
    sf::Sprite sp(texWhirlwind);
    sp.setRotation(std::fmod(t * 120.f, 360.f));
    float sx = float(TILE) / texWhirlwind.getSize().x;
    float sy = float(TILE) / texWhirlwind.getSize().y;
    sp.setScale(sx, sy);
    sp.setOrigin(texWhirlwind.getSize().x * 0.5f, texWhirlwind.getSize().y * 0.5f);
    sf::Vector2f p = CellToPx(gx, gy);
    sp.setPosition(p.x + TILE * 0.5f, p.y + TILE * 0.5f);
    win.draw(sp);
}

// ----------------- INIT -----------------
bool Gfx_Init(sf::RenderWindow& window) {
    window.setVerticalSyncEnabled(true);

    // Gọi UI Init
    Ui_Init(window);

    if (!gFont.loadFromFile("assets/fonts/RobotoMono-Regular.ttf")) std::cerr << "ERR: Font not found!\n";

    gHud.setFont(gFont); gHud.setCharacterSize(18); gHud.setFillColor(sf::Color(220, 220, 220));
    gSeg.setFont(gFont); gSeg.setCharacterSize(TILE - 12); gSeg.setFillColor(sf::Color::White);
    gHeadRect.setSize({ (float)TILE - 2.f,(float)TILE - 2.f });
    gBodyRect.setSize({ (float)TILE - 2.f,(float)TILE - 2.f });
    gFoodShape = sf::CircleShape((float)TILE * 0.35f); gFoodShape.setFillColor(sf::Color(220, 160, 40));
    gObsRect.setSize({ (float)TILE - 2.f,(float)TILE - 2.f }); gObsRect.setFillColor(sf::Color(170, 70, 70));

    if (!gEatBuf.loadFromFile("assets/sfx/eat.wav")) {}
    else gEat.setBuffer(gEatBuf);
    if (!gDieBuf.loadFromFile("assets/sfx/die.wav")) {}
    else gDie.setBuffer(gDieBuf);

    if (gRockTexture.loadFromFile("assets/UI/Rock.png")) { gRockSprite.setTexture(gRockTexture); gRockLoaded = true; }
    if (texWhirlwind.loadFromFile("assets/UI/Whirlwind.png")) { texWhirlwindLoaded = true; }
    if (texTorii.loadFromFile("assets/Torii.png")) { sprTorii.setTexture(texTorii); toriiLoaded = true; }
    if (gGameOverBgTexture.loadFromFile("assets/UI/GameOver.png")) { gGameOverBgSprite.setTexture(gGameOverBgTexture); gGameOverBgLoaded = true; }

    const char* bgPaths[3] = { "assets/Biomes/bg1.png", "assets/Biomes/bg2.png", "assets/Biomes/bg3.png" };
    for (int i = 0; i < 3; ++i) { if (gBgTextures[i].loadFromFile(bgPaths[i])) { gBgSprites[i].setTexture(gBgTextures[i]); gBgLoaded[i] = true; } }
    return true;
}

// ----------------- DRAW FRAME -----------------
void Gfx_DrawFrame(sf::RenderWindow& window) {
    const int WIN_W = (BOARD_W + BORDER * 2) * TILE;
    const int WIN_H = (BOARD_H + BORDER * 2) * TILE;

    int ev = Game_ConsumeEvents();
    if ((ev & 1) && Ui_AudioEnabled()) gEat.play();
    if ((ev & 2) && Ui_AudioEnabled()) gDie.play();

    window.clear(sf::Color(20, 20, 25));

    // 1. Background
    int level = Game_Level();
    int idx = (level - 1) % 3;
    if (idx >= 0 && idx < 3 && gBgLoaded[idx]) {
        sf::Vector2u texSize = gBgTextures[idx].getSize();
        float scaleX = (float)WIN_W / texSize.x;
        float scaleY = (float)WIN_H / texSize.y;
        float scale = std::max(scaleX, scaleY);
        gBgSprites[idx].setScale(scale, scale);
        float posX = (WIN_W - texSize.x * scale) * 0.5f;
        float posY = (WIN_H - texSize.y * scale) * 0.5f;
        gBgSprites[idx].setPosition(posX, posY);
        window.draw(gBgSprites[idx]);
    }

    // 2. Grid & Walls
    sf::RectangleShape lineX({ (float)WIN_W, 1.f }); lineX.setFillColor(sf::Color(40, 40, 45));
    sf::RectangleShape lineY({ 1.f, (float)WIN_H }); lineY.setFillColor(sf::Color(40, 40, 45));
    for (int i = 0; i <= BOARD_H; i++) { lineX.setPosition(0, (float)(i + BORDER) * TILE); window.draw(lineX); }
    for (int i = 0; i <= BOARD_W; i++) { lineY.setPosition((float)(i + BORDER) * TILE, 0); window.draw(lineY); }

    sf::Color wallColor(40, 40, 45);
    sf::RectangleShape bdr; bdr.setFillColor(wallColor);
    bdr.setSize({ (float)WIN_W, (float)BORDER * TILE }); bdr.setPosition(0, 0); window.draw(bdr); // Top
    bdr.setPosition(0, (float)(BORDER + BOARD_H) * TILE); window.draw(bdr); // Bottom
    bdr.setSize({ (float)BORDER * TILE, (float)BOARD_H * TILE }); bdr.setPosition(0, (float)BORDER * TILE); window.draw(bdr); // Left
    bdr.setPosition((float)(BORDER + BOARD_W) * TILE, (float)BORDER * TILE); window.draw(bdr); // Right

    // 3. Food
    Cell f = Game_Food();
    if (f.x >= 0 && f.y >= 0) {
        auto pf = CellToPx(f.x, f.y);
        gFoodShape.setPosition(pf.x + (float)TILE * 0.15f, pf.y + (float)TILE * 0.15f);
        window.draw(gFoodShape);
    }

    // 4. Obstacles
    for (std::size_t i = 0; i < Game_ObstacleCount(); ++i) {
        Cell o = Game_Obstacle(i);
        auto p = CellToPx(o.x, o.y);
        if (gRockLoaded) {
            gRockSprite.setOrigin(gRockTexture.getSize().x * 0.5f, gRockTexture.getSize().y * 0.5f);
            float scale = (float)(TILE - 2) / gRockTexture.getSize().x; scale *= 1.5f;
            gRockSprite.setScale(scale, scale); gRockSprite.setPosition(p.x + TILE * 0.5f, p.y + TILE * 0.5f);
            window.draw(gRockSprite);
        }
        else {
            gObsRect.setPosition(p.x + 1.f, p.y + 1.f); window.draw(gObsRect);
        }
    }

    // 5. Tornadoes
    static sf::Clock clock; float dt = clock.restart().asSeconds(); static float tTornado = 0.f; tTornado += dt;
    if (texWhirlwindLoaded) {
        for (size_t i = 0; i < Game_WhirlwindCount(); ++i) { Cell c = Game_Whirlwind(i); DrawTornado(window, c.x, c.y, tTornado); }
    }

    // 6. VẼ RẮN (VẼ TRƯỚC CỔNG)
    bool first = true;
    std::size_t len = Game_SnakeLen();
    std::string mssv = MSSV_STRING;
    bool isGhost = Game_IsGhost();

    // --- LẤY TRẠNG THÁI CHUI CỔNG ---
    bool isExiting = Game_IsExiting();
    int gateX = Game_PortalPos().x;

    for (std::size_t i = 0; i < len; ++i) {
        Cell c = Game_SnakeSeg(i);

        // --- LOGIC QUAN TRỌNG: ẨN ĐỐT RẮN ---
        // Nếu đang chui VÀ tọa độ đốt rắn lớn hơn hoặc bằng cổng -> KHÔNG VẼ
        if (isExiting && c.x >= gateX) {
            continue;
        }
        // ------------------------------------

        auto p = CellToPx(c.x, c.y);
        sf::Color col = SkinColorFor(i, len, first);
        sf::RectangleShape* proto = first ? &gHeadRect : &gBodyRect;
        sf::RectangleShape rect = *proto;
        rect.setFillColor(col);
        rect.setPosition(p.x + 1.f, p.y + 1.f);

        if (isGhost) {
            // Hiệu ứng Ghost (mờ + phát sáng)
            for (int g = 0; g < 2; ++g) {
                float scale = 1.15f + g * 0.12f;
                sf::RectangleShape glow = rect;
                glow.setSize(sf::Vector2f((float)TILE * scale, (float)TILE * scale));
                glow.setPosition(p.x + 1.f - ((float)TILE * (scale - 1.f) / 2.f), p.y + 1.f - ((float)TILE * (scale - 1.f) / 2.f));
                sf::Uint8 alpha = (sf::Uint8)std::max(0, 60 - g * 20);
                glow.setFillColor(sf::Color(120, 220, 255, alpha));
                window.draw(glow);
            }
            sf::RectangleShape fade = rect;
            fade.setFillColor(sf::Color(col.r, col.g, col.b, 140));
            window.draw(fade);
        }
        else {
            // Vẽ rắn bình thường
            window.draw(rect);
        }

        // Vẽ số MSSV
        if (!mssv.empty()) {
            char ch = mssv[i % mssv.size()];
            gSeg.setString(sf::String(ch));
            sf::FloatRect b = gSeg.getLocalBounds();
            gSeg.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
            if (isGhost) {
                sf::Color txtCol = gSeg.getFillColor(); gSeg.setFillColor(sf::Color(txtCol.r, txtCol.g, txtCol.b, 160));
                gSeg.setPosition(p.x + (float)TILE / 2.f, p.y + (float)TILE / 2.f); window.draw(gSeg);
                gSeg.setFillColor(sf::Color::White);
            }
            else {
                gSeg.setPosition(p.x + (float)TILE / 2.f, p.y + (float)TILE / 2.f); window.draw(gSeg);
            }
        }
        first = false;
    }

    // 7. VẼ CỔNG SAU CÙNG (ĐỂ ĐÈ LÊN RẮN NẾU CẦN)
    if (Game_IsAutoPilot()) {
        Cell gate = Game_PortalPos();
        if (gate.x >= 0) {
            sf::Vector2f p = CellToPx(gate.x, gate.y);

            if (toriiLoaded) {
                sf::Vector2f center = p;
                center.x += TILE * 0.5f; center.y += TILE * 0.5f;
                sprTorii.setScale(0.20f, 0.20f);
                if (sprTorii.getTexture()) {
                    sprTorii.setOrigin(static_cast<float>(sprTorii.getTexture()->getSize().x) / 2.0f, static_cast<float>(sprTorii.getTexture()->getSize().y));
                }
                sprTorii.setPosition(center.x, center.y + TILE * 0.70f);
                window.draw(sprTorii);
            }
            else {
                static sf::CircleShape gateCircle;
                gateCircle.setRadius(TILE * 0.5f); gateCircle.setOrigin(TILE * 0.5f, TILE * 0.5f);
                gateCircle.setPosition(p.x + TILE * 0.5f, p.y + TILE * 0.5f);
                static sf::Clock blinkClock; float t = blinkClock.getElapsedTime().asSeconds();
                if (std::fmod(t, 0.4f) < 0.2f) gateCircle.setFillColor(sf::Color(80, 220, 220, 180));
                else gateCircle.setFillColor(sf::Color(30, 150, 200, 160));
                window.draw(gateCircle);
            }
        }
    }

    // 8. HUD & Overlays
    std::string profileName = Game_GetProfileName();
    gHud.setString("PLAYER: " + profileName + " | SCORE: " + std::to_string(Game_Score()) + " | LVL: " + std::to_string(Game_Level()));
    gHud.setPosition(10.f, 5.f);
    window.draw(gHud);

    if (Game_Over()) {
        if (gGameOverBgLoaded) {
            sf::Vector2u bgSize = gGameOverBgTexture.getSize();
            float scaleX = (float)WIN_W / bgSize.x; float scaleY = (float)WIN_H / bgSize.y;
            float scale = std::max(scaleX, scaleY);
            gGameOverBgSprite.setScale(scale, scale);
            gGameOverBgSprite.setPosition((WIN_W - bgSize.x * scale) * 0.5f, (WIN_H - bgSize.y * scale) * 0.5f);
            window.draw(gGameOverBgSprite);
        }
        else {
            sf::RectangleShape fov({ (float)WIN_W, (float)WIN_H }); fov.setFillColor(sf::Color(0, 0, 0, 180)); window.draw(fov);
        }
        Gfx_DrawGameOverOverlay(window);
    }

    if (Game_Paused()) {
        Gfx_DrawPauseMenu(window);
    }

    window.display();
}