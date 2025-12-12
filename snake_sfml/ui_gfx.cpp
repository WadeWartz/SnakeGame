#include "ui_gfx.h"
#include "game_render.h" // <- ensure enum MenuHit is defined here
#include "config.h"
#include "game.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// ----------------- UI GLOBALS (textures/fonts/sprites used by menus) -----------------
static sf::Font gFont;
static sf::Text gHud; // may be used by some UI screens (small HUD text)

// Main menu images
static sf::Texture gMainMenuBgTexture;
static sf::Sprite  gMainMenuBgSprite;
static bool        gMainMenuBgLoaded = false;

static sf::Texture gTitleTexture;
static sf::Sprite  gTitleSprite;
static bool        gTitleImageLoaded = false;

// Buttons images (main menu)
static sf::Texture gBtnStartTexture, gBtnOptionTexture, gBtnQuitTexture;
static sf::Sprite  gBtnStartSprite, gBtnOptionSprite, gBtnQuitSprite;
static bool        gButtonImagesLoaded = false;

// Pause background / title
static sf::Texture gPauseScreenBgTexture;
static sf::Sprite  gPauseScreenBgSprite;
static bool        gPauseScreenBgLoaded = false;
static sf::Texture gPauseTitleTexture;
static sf::Sprite  gPauseTitleSprite;
static bool        gPauseTitleLoaded = false;

// Pause buttons images (if available)
static sf::Texture gBtnResumeTexture, gBtnRestartTexture, gBtnExitTexture, gBtnSaveTexture;
static sf::Sprite  gBtnResumeSprite, gBtnRestartSprite, gBtnExitSprite, gBtnSaveSprite;
static bool        gPauseButtonImagesLoaded = false;

// Profile menu images
static sf::Texture gBtnNewSaveTexture, gBtnLoadSaveTexture, gBtnBackTexture, gSaveSlotsTexture;
static sf::Sprite  gBtnNewSaveSprite, gBtnLoadSaveSprite, gBtnBackSprite, gSaveSlotsSprite;
static bool        gProfileButtonImagesLoaded = false;
static bool        gSaveSlotsLoaded = false;

// --- Add these globals near the other UI textures/sprites (in ui_gfx.cpp global section) ---
static sf::Texture gEnterNameTexture;
static sf::Sprite  gEnterNameSprite;
static bool        gEnterNameLoaded = false;

// Options screen background (new)
static sf::Texture gOptionsBgTexture;
static sf::Sprite  gOptionsBgSprite;
static bool        gOptionsBgLoaded = false;

// Options title image (replaces textual "OPTIONS")
static sf::Texture gOptionsTitleTexture;
static sf::Sprite  gOptionsTitleSprite;
static bool        gOptionsTitleLoaded = false;

// New assets requested by designer
static sf::Texture gSkinSelectionTexture;
static sf::Sprite  gSkinSelectionSprite;
static bool        gSkinSelectionLoaded = false;

static sf::Texture gAudioConfigTexture;
static sf::Sprite  gAudioConfigSprite;
static bool        gAudioConfigLoaded = false;

static sf::Texture gBack1Texture;
static sf::Sprite  gBack1Sprite;
static bool        gBack1Loaded = false;
static sf::FloatRect rOpt_Back;
static float hOptBack = 0.f;

// SaveRects for main menu button hit testing
static sf::FloatRect gBtnPlayR, gBtnOptR, gBtnQuitR;

// Pause menu rects + hover state
static float hP_Resume = 0, hP_Restart = 0, hP_Save = 0, hP_Load = 0, hP_Exit = 0;
static sf::FloatRect rP_Resume, rP_Restart, rP_Save, rP_Load, rP_Exit;

// Profile menu rects/hover
static sf::FloatRect rProNew, rProChoose, rProBack;
static float hProNew = 0, hProChoose = 0, hProBack = 0;

// --- Game Over UI globals ---
static sf::Texture gGameOverTitleTexture;
static sf::Sprite  gGameOverTitleSprite;
static bool        gGameOverTitleLoaded = false;

// Game Over full-screen background
static sf::Texture gGameOverBgTexture;
static sf::Sprite  gGameOverBgSprite;
static bool        gGameOverBgLoaded = false;

// Use distinct textures/sprites for Game Over buttons to avoid conflicts with Pause buttons
static sf::Texture gBtnExit1Texture, gBtnRestart1Texture;
static sf::Sprite  gBtnExit1Sprite, gBtnRestart1Sprite;
static bool        gGameOverBtnLoaded = false;

static sf::FloatRect rGO_Restart, rGO_Exit;
static float hGO_Restart = 0.f, hGO_Exit = 0.f;

// --- Add near top UI globals ---
static bool gAudioEnabled = true; // default on

void Ui_SetAudioEnabled(bool enabled) { gAudioEnabled = enabled; }
bool Ui_AudioEnabled() { return gAudioEnabled; }

// simple helper math
static float s_lerp(float a, float b, float t) { return a + (b - a) * t; }
static float s_saturate(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
static float s_smooth(float t) { return t * t * (3.f - 2.f * t); }

// ----------------- UI HELPERS -----------------
static void drawVignette(sf::RenderWindow& win) {
    sf::Vector2u sz = win.getSize();
    sf::Vertex quad[4];
    quad[0].position = { 0.f, 0.f };
    quad[1].position = { (float)sz.x, 0.f };
    quad[2].position = { (float)sz.x, (float)sz.y };
    quad[3].position = { 0.f, (float)sz.y };

    sf::Color cEdge(5, 5, 10, 255);
    quad[0].color = cEdge; quad[1].color = cEdge; quad[2].color = cEdge; quad[3].color = cEdge;
    win.draw(quad, 4, sf::PrimitiveType::Quads);

    sf::RectangleShape border({ (float)sz.x - 16.f, (float)sz.y - 16.f });
    border.setPosition(8.f, 8.f);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(2.f);
    border.setOutlineColor(sf::Color(40, 60, 80));
    win.draw(border);
}

static void drawRoundBox(sf::RenderWindow& win, const sf::FloatRect& r, float radius,
    sf::Color fill, sf::Color outline = sf::Color::Transparent, float ot = 0.f,
    bool shadow = true)
{
    float x = r.left, y = r.top, w = r.width, h = r.height, rd = radius;
    if (shadow) {
        sf::RectangleShape sh({ w, h });
        sh.setPosition(x + 4.f, y + 4.f);
        sh.setFillColor(sf::Color(0, 0, 0, 100));
        win.draw(sh);
    }

    sf::RectangleShape body({ w - 2 * rd, h });
    body.setPosition(x + rd, y);
    body.setFillColor(fill);
    win.draw(body);

    sf::RectangleShape side({ rd, h - 2 * rd });
    side.setFillColor(fill);
    side.setPosition(x, y + rd); win.draw(side);
    side.setPosition(x + w - rd, y + rd); win.draw(side);

    sf::CircleShape arc(rd);
    arc.setFillColor(fill);
    arc.setPosition(x, y); win.draw(arc);
    arc.setPosition(x + w - 2 * rd, y); win.draw(arc);
    arc.setPosition(x, y + h - 2 * rd); win.draw(arc);
    arc.setPosition(x + w - 2 * rd, y + h - 2 * rd); win.draw(arc);

    if (ot > 0.f) {
        sf::RectangleShape b2({ w + ot * 2, h + ot * 2 });
        b2.setPosition(x - ot, y - ot);
        b2.setFillColor(sf::Color::Transparent);
        b2.setOutlineThickness(ot);
        b2.setOutlineColor(outline);
        win.draw(b2);
    }
}

static sf::Clock sMenuClock;
static void drawPrettyButton(sf::RenderWindow& win, const sf::FloatRect& baseR,
    const char* label, float& hover01)
{
    sf::Vector2i m = sf::Mouse::getPosition(win);
    sf::Vector2f mf((float)m.x, (float)m.y);
    bool inside = baseR.contains(mf);

    float target = inside ? 1.f : 0.f;
    hover01 += (target - hover01) * 0.15f;
    float t = s_smooth(hover01);

    float sx = s_lerp(1.f, 1.05f, t);
    float sy = s_lerp(1.f, 1.05f, t);

    sf::FloatRect r = baseR;
    float dw = r.width * (sx - 1.f);
    float dh = r.height * (sy - 1.f);
    r.left -= dw / 2.f;
    r.top -= dh / 2.f;
    r.width *= sx;
    r.height *= sy;

    sf::Color cNormal(40, 50, 60);
    sf::Color cHover(70, 100, 140);
    sf::Color fill = sf::Color(
        (sf::Uint8)s_lerp((float)cNormal.r, (float)cHover.r, t),
        (sf::Uint8)s_lerp((float)cNormal.g, (float)cHover.g, t),
        (sf::Uint8)s_lerp((float)cNormal.b, (float)cHover.b, t)
    );

    drawRoundBox(win, r, 10.f, fill, sf::Color(200, 200, 200), inside ? 2.f : 0.f);

    sf::Text txt(label, gFont, 24);
    txt.setFillColor(sf::Color(230, 230, 230));
    sf::FloatRect lb = txt.getLocalBounds();
    txt.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height / 2.f);
    txt.setPosition(r.left + r.width / 2.f, r.top + r.height / 2.f);
    win.draw(txt);
}

// ----------------- INIT (UI) -----------------
// Change the return type of Ui_Init to void to match the previous declaration/definition
void Ui_Init(sf::RenderWindow& window) {
    if (!gFont.loadFromFile("assets/fonts/RobotoMono-Regular.ttf")) {
        std::cerr << "UI: Font not found!\n";
    }
    gHud.setFont(gFont);
    gHud.setCharacterSize(18);
    gHud.setFillColor(sf::Color(220, 220, 220));

    // Main menu background & title
    if (gMainMenuBgTexture.loadFromFile("assets/UI/MainScreen.png")) {
        gMainMenuBgSprite.setTexture(gMainMenuBgTexture);
        gMainMenuBgTexture.setSmooth(false);
        gMainMenuBgLoaded = true;
    }
    if (gTitleTexture.loadFromFile("assets/UI/SnakeGame.png")) {
        gTitleSprite.setTexture(gTitleTexture);
        gTitleTexture.setSmooth(false);
        gTitleImageLoaded = true;
    }

    // Main menu buttons
    bool startLoaded = gBtnStartTexture.loadFromFile("assets/UI/Start.png");
    bool optLoaded = gBtnOptionTexture.loadFromFile("assets/UI/Option.png");
    bool quitLoaded = gBtnQuitTexture.loadFromFile("assets/UI/Quit.png");
    if (startLoaded)  gBtnStartSprite.setTexture(gBtnStartTexture);
    if (optLoaded)    gBtnOptionSprite.setTexture(gBtnOptionTexture);
    if (quitLoaded)   gBtnQuitSprite.setTexture(gBtnQuitTexture);
    gButtonImagesLoaded = (startLoaded && optLoaded && quitLoaded);

    // Pause images (optional)
    bool resumeLoaded = gBtnResumeTexture.loadFromFile("assets/UI/Resume.png");
    bool restartLoaded = gBtnRestartTexture.loadFromFile("assets/UI/Restart.png");
    bool exitLoaded = gBtnExitTexture.loadFromFile("assets/UI/Exit.png");
    bool saveLoaded = gBtnSaveTexture.loadFromFile("assets/UI/Save.png"); // new Save.png for pause
    if (resumeLoaded)  gBtnResumeSprite.setTexture(gBtnResumeTexture);
    if (restartLoaded) gBtnRestartSprite.setTexture(gBtnRestartTexture);
    if (exitLoaded)    gBtnExitSprite.setTexture(gBtnExitTexture);
    if (saveLoaded)    gBtnSaveSprite.setTexture(gBtnSaveTexture);
    gPauseButtonImagesLoaded = (resumeLoaded && restartLoaded && exitLoaded && saveLoaded);

    // Pause background/title
    if (gPauseScreenBgTexture.loadFromFile("assets/UI/PauseScreen.png")) {
        gPauseScreenBgSprite.setTexture(gPauseScreenBgTexture);
        gPauseScreenBgLoaded = true;
    }
    if (gPauseTitleTexture.loadFromFile("assets/UI/Paused.png")) {
        gPauseTitleSprite.setTexture(gPauseTitleTexture);
        gPauseTitleLoaded = true;
    }

    // Options background (new)
    if (gOptionsBgTexture.loadFromFile("assets/UI/OptionScreen.png")) {
        gOptionsBgSprite.setTexture(gOptionsBgTexture);
        gOptionsBgTexture.setSmooth(false); // <- disable smoothing for pixel-crisp rendering
        gOptionsBgLoaded = true;
    }

    // Options title image (left-top)
    if (gOptionsTitleTexture.loadFromFile("assets/UI/GameSettings.png")) {
        gOptionsTitleSprite.setTexture(gOptionsTitleTexture);
        gOptionsTitleTexture.setSmooth(false);
        gOptionsTitleLoaded = true;
    }

    // New option assets
    if (gSkinSelectionTexture.loadFromFile("assets/UI/SkinSelection.png")) {
        gSkinSelectionSprite.setTexture(gSkinSelectionTexture, true); // reset rect to full texture
        gSkinSelectionTexture.setSmooth(false);
        gSkinSelectionSprite.setOrigin(0.f, 0.f);
        gSkinSelectionLoaded = true;
    }
    if (gAudioConfigTexture.loadFromFile("assets/UI/AudioConfig.png")) {
        gAudioConfigSprite.setTexture(gAudioConfigTexture, true); // reset rect to full texture
        gAudioConfigTexture.setSmooth(false);
        gAudioConfigSprite.setOrigin(0.f, 0.f);
        gAudioConfigLoaded = true;
    }
    if (gBack1Texture.loadFromFile("assets/UI/Back1.png")) {
        gBack1Sprite.setTexture(gBack1Texture, true); // reset rect to full texture
        gBack1Texture.setSmooth(false);
        gBack1Sprite.setOrigin(0.f, 0.f);
        gBack1Loaded = true;
    }

    // Title image for name input screen (optional)
    if (gEnterNameTexture.loadFromFile("assets/UI/EnterYourName.png")) {
        gEnterNameSprite.setTexture(gEnterNameTexture);
        gEnterNameTexture.setSmooth(false);
        gEnterNameLoaded = true;
    }

    // Profile menu images
    bool newSaveLoaded = gBtnNewSaveTexture.loadFromFile("assets/UI/NewSave.png");
    bool loadSaveLoaded = gBtnLoadSaveTexture.loadFromFile("assets/UI/LoadSave.png");
    bool backLoaded = gBtnBackTexture.loadFromFile("assets/UI/Back.png");
    if (newSaveLoaded)   gBtnNewSaveSprite.setTexture(gBtnNewSaveTexture);
    if (loadSaveLoaded)  gBtnLoadSaveSprite.setTexture(gBtnLoadSaveTexture);
    if (backLoaded)      gBtnBackSprite.setTexture(gBtnBackTexture);
    gProfileButtonImagesLoaded = (newSaveLoaded && loadSaveLoaded && backLoaded);

    if (gSaveSlotsTexture.loadFromFile("assets/UI/SaveSlots.png")) {
        gSaveSlotsSprite.setTexture(gSaveSlotsTexture);
        gSaveSlotsTexture.setSmooth(false);
        gSaveSlotsLoaded = true;
    }

    // --- NEW: Game Over title + buttons + background ---
    if (gGameOverTitleTexture.loadFromFile("assets/UI/GameOver.png")) {
        gGameOverTitleSprite.setTexture(gGameOverTitleTexture);
        gGameOverTitleTexture.setSmooth(false);
        gGameOverTitleLoaded = true;
    }
    if (gGameOverBgTexture.loadFromFile("assets/UI/GameOverScreen.png")) {
        gGameOverBgSprite.setTexture(gGameOverBgTexture);
        gGameOverBgLoaded = true;
    }

    // Load distinct Game Over buttons (try Restart1.png and Exit1.png). If not present, fall back to pause sprites.
    bool exitLoadedGO = gBtnExit1Texture.loadFromFile("assets/UI/Exit1.png");
    bool restartLoadedGO = gBtnRestart1Texture.loadFromFile("assets/UI/Restart1.png");
    if (exitLoadedGO)    gBtnExit1Sprite.setTexture(gBtnExit1Texture);
    if (restartLoadedGO) gBtnRestart1Sprite.setTexture(gBtnRestart1Texture);

    // fallback: reuse pause sprites if game-over specific ones missing
    if (!restartLoadedGO && restartLoaded) {
        gBtnRestart1Sprite = gBtnRestartSprite;
        restartLoadedGO = true;
    }
    if (!exitLoadedGO && exitLoaded) {
        gBtnExit1Sprite = gBtnExitSprite;
        exitLoadedGO = true;
    }
    // consider loaded if at least one button available (we draw each conditionally)
    gGameOverBtnLoaded = (exitLoadedGO || restartLoadedGO);

    // initial layout
    Gfx_MenuLayout((BOARD_W + BORDER * 2) * TILE, (BOARD_H + BORDER * 2) * TILE);
    // Pause layout (4 items now: Resume, Restart, Save, Exit)
    float w = 300.f, h = 50.f, gap = 15.f;
    float totalH = 4 * h + 3 * gap;
    float startY = (((BOARD_H + BORDER * 2) * TILE) - totalH) / 2.f;
    float centerX = ((BOARD_W + BORDER * 2) * TILE) / 2.f - w / 2.f;
    rP_Resume = { centerX, startY, w, h };
    rP_Restart = { centerX, startY + (h + gap) * 1, w, h };
    rP_Save = { centerX, startY + (h + gap) * 2, w, h };
    rP_Exit = { centerX, startY + (h + gap) * 3, w, h };

    // Setup Game Over button rects (vertical stack: Restart above Exit, both centered)
    const int WIN_W = (BOARD_W + BORDER * 2) * TILE;
    const int WIN_H = (BOARD_H + BORDER * 2) * TILE;
    float goW = 260.f, goH = 64.f, goGap = 18.f;
    float centerGX = (float)(WIN_W - goW) * 0.5f;
    float centerGY = (float)WIN_H * 0.52f;
    rGO_Restart = { centerGX, centerGY - goH - goGap * 0.5f, goW, goH };
    rGO_Exit = { centerGX, centerGY + goGap * 0.5f,              goW, goH };
}

// ----------------- MENU LAYOUT -----------------
void Gfx_MenuLayout(int ww, int wh)
{
    if (gButtonImagesLoaded) {
        sf::Vector2u startSize = gBtnStartTexture.getSize();
        sf::Vector2u optionSize = gBtnOptionTexture.getSize();
        sf::Vector2u quitSize = gBtnQuitTexture.getSize();

        float scale = std::min(ww / 1200.f, wh / 900.f);
        scale = std::max(0.35f, std::min(scale, 0.7f));

        float btnW = startSize.x * scale;
        float btnH = startSize.y * scale;
        float optionW = optionSize.x * scale;
        float optionH = optionSize.y * scale;
        float quitW = quitSize.x * scale;
        float quitH = quitSize.y * scale;

        float startY = wh * 0.35f;
        float gap = 30.f;

        gBtnPlayR = { (ww - btnW) * 0.5f, startY,                 btnW,    btnH };
        gBtnOptR = { (ww - optionW) * 0.5f, startY + btnH + gap,    optionW, optionH };
        gBtnQuitR = { (ww - quitW) * 0.5f, startY + (btnH + gap) * 2, quitW,  quitH };
    }
    else {
        float panelW = std::min<float>(420.f, ww * 0.6f);
        float panelX = (ww - panelW) * 0.5f;
        float y = wh * 0.28f;
        float h = 56.f;
        float gap = 18.f;

        gBtnPlayR = { panelX, y,                 panelW, h };
        gBtnOptR = { panelX, y + h + gap,       panelW, h };
        gBtnQuitR = { panelX, y + (h + gap) * 2, panelW, h };
    }
}

const sf::FloatRect& Gfx_BtnPlay() { return gBtnPlayR; }
const sf::FloatRect& Gfx_BtnOptions() { return gBtnOptR; }
const sf::FloatRect& Gfx_BtnQuit() { return gBtnQuitR; }

// ----------------- MAIN MENU DRAW -----------------
static void drawButton(sf::RenderWindow& win, const sf::FloatRect& r,
    const char* text, bool hover = false)
{
    sf::RectangleShape box({ r.width, r.height });
    box.setPosition(r.left, r.top);
    box.setFillColor(hover ? sf::Color(60, 60, 60, 200) : sf::Color(45, 45, 45, 180));
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(80, 80, 80));
    win.draw(box);

    sf::Text t(text, gFont, 24);
    t.setFillColor(sf::Color(230, 230, 230));
    sf::FloatRect lb = t.getLocalBounds();
    t.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height / 2.f);
    t.setPosition(r.left + r.width / 2.f, r.top + r.height / 2.f);
    win.draw(t);
}

void Gfx_DrawMainMenu(sf::RenderWindow& window, int previewSkin)
{
    sf::Vector2u winSize = window.getSize();

    if (gMainMenuBgLoaded) {
        sf::Vector2u texSize = gMainMenuBgTexture.getSize();
        float scaleX = (float)winSize.x / texSize.x;
        float scaleY = (float)winSize.y / texSize.y;
        float scale = std::max(scaleX, scaleY);
        gMainMenuBgSprite.setScale(scale, scale);
        float posX = (winSize.x - texSize.x * scale) * 1.0f;
        float posY = (winSize.y - texSize.y * scale) * 0.4f;
        gMainMenuBgSprite.setPosition(posX, posY);
        window.draw(gMainMenuBgSprite);
    }

    if (gTitleImageLoaded) {
        float titleScale = std::min(winSize.x / 1000.f, winSize.y / 800.f);
        titleScale = std::max(0.3f, std::min(titleScale, 0.8f));
        sf::Vector2u titleSize = gTitleTexture.getSize();
        gTitleSprite.setScale(titleScale, titleScale);
        float titleW = titleSize.x * titleScale;
        gTitleSprite.setPosition((winSize.x - titleW) * 0.5f, winSize.y * 0.1f);
        window.draw(gTitleSprite);
    }

    auto mouse = sf::Mouse::getPosition(window);
    sf::Vector2f m((float)mouse.x, (float)mouse.y);

    if (gButtonImagesLoaded) {
        bool hoverStart = gBtnPlayR.contains(m);
        bool hoverOption = gBtnOptR.contains(m);
        bool hoverQuit = gBtnQuitR.contains(m);

        float scale = std::min(winSize.x / 1200.f, winSize.y / 900.f);
        scale = std::max(0.35f, std::min(scale, 0.7f));

        float startScale = scale * (hoverStart ? 1.1f : 1.0f);
        gBtnStartSprite.setScale(startScale, startScale);
        sf::Vector2u startSize = gBtnStartTexture.getSize();
        float startW = startSize.x * startScale;
        gBtnStartSprite.setPosition((winSize.x - startW) * 0.5f, gBtnPlayR.top);
        gBtnStartSprite.setColor(hoverStart ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnStartSprite);

        float optionScale = scale * (hoverOption ? 1.1f : 1.0f);
        gBtnOptionSprite.setScale(optionScale, optionScale);
        sf::Vector2u optionSize = gBtnOptionTexture.getSize();
        float optionW = optionSize.x * optionScale;
        gBtnOptionSprite.setPosition((winSize.x - optionW) * 0.5f, gBtnOptR.top);
        gBtnOptionSprite.setColor(hoverOption ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnOptionSprite);

        float quitScale = scale * (hoverQuit ? 1.1f : 1.0f);
        gBtnQuitSprite.setScale(quitScale, quitScale);
        sf::Vector2u quitSize = gBtnQuitTexture.getSize();
        float quitW = quitSize.x * quitScale;
        gBtnQuitSprite.setPosition((winSize.x - quitW) * 0.5f, gBtnQuitR.top);
        gBtnQuitSprite.setColor(hoverQuit ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnQuitSprite);
    }
}

// ----------------- OPTIONS -----------------
void Gfx_DrawOptions(sf::RenderWindow& window, int previewSkin)
{
    sf::Vector2u winSize = window.getSize();

    // If options background image is available draw it; otherwise clear to dark color
    if (gOptionsBgLoaded) {
        sf::Vector2u texSize = gOptionsBgTexture.getSize();
        float scaleX = (float)winSize.x / texSize.x;
        float scaleY = (float)winSize.y / texSize.y;
        float scale = std::max(scaleX, scaleY);
        gOptionsBgSprite.setScale(scale, scale);
        float posX = (winSize.x - texSize.x * scale) * 0.5f;
        float posY = (winSize.y - texSize.y * scale) * 0.5f;
        gOptionsBgSprite.setPosition(posX, posY);
        window.draw(gOptionsBgSprite);
    }
    else {
        window.clear(sf::Color(25, 25, 25));
    }

    // Left margin for left-aligned layout
    const float leftX = 40.f;

    // Title (left-aligned) - prefer image if available
    if (gOptionsTitleLoaded) {
        sf::Vector2u ts = gOptionsTitleTexture.getSize();
        float titleMaxH = winSize.y * 0.12f; // target max height
        float titleScale = std::min(1.f, titleMaxH / (float)ts.y);
        gOptionsTitleSprite.setScale(titleScale, titleScale);
        float titleY = winSize.y * 0.06f; // a bit down from the top
        gOptionsTitleSprite.setPosition(std::round(leftX), std::round(titleY));
        window.draw(gOptionsTitleSprite);
    }
    else {
        sf::Text title("OPTIONS", gFont, 42);
        title.setFillColor(sf::Color(230, 230, 230));
        title.setPosition(leftX, winSize.y * 0.08f);
        window.draw(title);
    }

    // Replace the textual hint with SkinSelection image when available (otherwise keep hint text)
    float cx = winSize.x * 0.5f; // kept for fallback color box placement if image missing
    float cy = winSize.y * 0.58f; // fallback location for color boxes

    if (gSkinSelectionLoaded) {
        sf::Vector2u ts = gSkinSelectionTexture.getSize();
        // reduce overall size so image is smaller than before
        float maxWidth = winSize.x * 0.45f; // narrower than previous 0.6
        float maxHeight = winSize.y * 0.10f; // keep height small
        float scaleW = maxWidth / (float)ts.x;
        float scaleH = maxHeight / (float)ts.y;
        float scale = std::min(scaleW, scaleH);
        // ensure not larger than 1
        scale = std::min(scale, 1.f);
        // small extra shrink to match designer request
        scale *= 0.7f;

        gSkinSelectionSprite.setScale(scale, scale);
        float imgW = ts.x * scale;
        float imgH = ts.y * scale;

        // Left-align the skin selection image at leftX
        float imgX = leftX;
        float imgY = winSize.y * 0.18f;
        gSkinSelectionSprite.setPosition(std::round(imgX), std::round(imgY)); // <- snap to integer pixels
        window.draw(gSkinSelectionSprite);

        // place color boxes directly under the skin selection image (left-aligned with it)
        cy = imgY + imgH + 18.f;
        // use cx for center fallback only; compute startX relative to imgX
        cx = imgX + 8.f;
    }
    else {
        sf::Text hint("Choose Snake Skin: 1..6   |   ENTER: Back   |   ESC: Back", gFont, 20);
        hint.setFillColor(sf::Color(200, 200, 200));
        hint.setPosition(leftX, winSize.y * 0.28f);
        window.draw(hint);

        // set color boxes start X to left margin + small offset
        cx = leftX + 8.f;
        cy = winSize.y * 0.34f;
    }

    // Skin picker (color boxes) - unchanged visuals but positioned using cy computed above
    sf::RectangleShape cell({ 30.f,30.f });
    auto colorFor = [&](int idx)->sf::Color {
        switch (idx % 6) {
        case 0: return sf::Color(0, 180, 140);
        case 1: return sf::Color(0, 205, 90);
        case 2: return sf::Color(0, 150, 220);
        case 3: return sf::Color(240, 90, 80);
        case 4: return sf::Color(200, 140, 40);
        default: return sf::Color(170, 90, 190);
        }
        };

    float startX = cx;
    for (int i = 0; i < 6; i++) {
        cell.setPosition(startX + i * 42.f, cy);
        cell.setFillColor(colorFor(i));
        cell.setOutlineThickness(i == previewSkin ? 3.f : 1.f);
        cell.setOutlineColor(i == previewSkin ? sf::Color::White : sf::Color(80, 80, 80));
        window.draw(cell);
    }

    // --- Audio toggle UI ---
    float audW = 300.f, audH = 44.f;
    // left align audio control under color boxes
    sf::Vector2f audPos(leftX, cy + 80.f);

    // Draw AudioConfig image above the audio control if available, left-aligned
    if (gAudioConfigLoaded) {
        sf::Vector2u ats = gAudioConfigTexture.getSize();
        float maxCfgW = audW;
        // base scale to fit into the audio control width
        float baseScale = std::min(maxCfgW / (float)ats.x, 1.f) * 0.9f;

        // Compute displayed integer pixel size to avoid sub-pixel mapping
        int dispW = std::lround(ats.x * baseScale);
        int dispH = std::lround(ats.y * baseScale);

        // Recompute exact per-axis scale so the sprite maps to integer pixels
        float scaleX = (float)dispW / (float)ats.x;
        float scaleY = (float)dispH / (float)ats.y;

        gAudioConfigSprite.setScale(scaleX, scaleY);
        // defensive: ensure full texture used and origin at top-left
        gAudioConfigSprite.setTextureRect(sf::IntRect(0, 0, (int)ats.x, (int)ats.y));
        gAudioConfigSprite.setOrigin(0.f, 0.f);

        float cfgW = (float)dispW;
        float cfgH = (float)dispH;
        float cfgX = leftX;
        float cfgY = audPos.y - cfgH - 12.f;

        // snap position to integer pixels as well
        gAudioConfigSprite.setPosition(std::round(cfgX), std::round(cfgY));
        window.draw(gAudioConfigSprite);
    }

    sf::FloatRect audRect(audPos.x, audPos.y, audW, audH);
    // Draw rounded box using helper
    drawRoundBox(window, audRect, 8.f, sf::Color(20, 20, 20), sf::Color(100, 100, 100), 2.f, true);

    // Caption - left aligned inside the box
    sf::Text audText(std::string("Audio: ") + (Ui_AudioEnabled() ? "ON" : "OFF"), gFont, 20);
    audText.setFillColor(sf::Color(220, 220, 220));
    // place text near left edge of the audio box
    audText.setPosition(audPos.x + 12.f, audPos.y + (audH - 24.f) * 0.5f - 2.f);
    window.draw(audText);

    // Visual switch (simple rectangle) placed at right side of the audio box
    float swW = 80.f, swH = 30.f;
    sf::RectangleShape swBg({ swW, swH });
    swBg.setPosition(audPos.x + audW - swW - 16.f, audPos.y + (audH - swH) * 0.5f);
    swBg.setFillColor(Ui_AudioEnabled() ? sf::Color(30, 180, 80) : sf::Color(90, 90, 90));
    swBg.setOutlineThickness(2.f);
    swBg.setOutlineColor(sf::Color(60, 60, 60));
    window.draw(swBg);

    sf::Text swLabel(Ui_AudioEnabled() ? "ON" : "OFF", gFont, 16);
    swLabel.setFillColor(sf::Color::White);
    sf::FloatRect slb = swLabel.getLocalBounds();
    swLabel.setOrigin(slb.left + slb.width / 2.f, slb.top + slb.height / 2.f);
    swLabel.setPosition(swBg.getPosition().x + swW * 0.5f, swBg.getPosition().y + swH * 0.5f);
    window.draw(swLabel);

    // Draw Back1 button at bottom-left (left-aligned) with hover-scale animation
    if (gBack1Loaded) {
        sf::Vector2u bts = gBack1Texture.getSize();
        float baseScale = std::min(winSize.x * 0.20f / (float)bts.x, 1.f);

        // base size & position (anchor bottom-left)
        float bw_base = bts.x * baseScale;
        float bh_base = bts.y * baseScale;
        float bx = leftX;
        float by_base = winSize.y - bh_base - 40.f;

        // mouse in window coords
        auto mpos = sf::Mouse::getPosition(window);
        sf::Vector2f mf((float)mpos.x, (float)mpos.y);

        // check hover against the base rect so pointer is detected before scale changes
        bool hoverBase = sf::FloatRect(bx, by_base, bw_base, bh_base).contains(mf);

        // smooth hover animation (hOptBack declared near globals)
        float target = hoverBase ? 1.f : 0.f;
        hOptBack += (target - hOptBack) * 0.15f;
        float t = s_smooth(hOptBack);
        float scaleMul = s_lerp(1.f, 1.06f, t);
        float dispScale = baseScale * scaleMul;

        // displayed size/pos using animated scale (keep left alignment)
        float bw = bts.x * dispScale;
        float bh = bts.y * dispScale;
        float by = winSize.y - bh - 40.f;

        gBack1Sprite.setScale(dispScale, dispScale);
        gBack1Sprite.setPosition(std::round(bx), std::round(by));

        // update clickable rect to match rendered sprite
        rOpt_Back = { bx, by, bw, bh };

        // tint like other buttons
        gBack1Sprite.setColor(hoverBase ? sf::Color::White : sf::Color(200, 200, 200));
        window.draw(gBack1Sprite);
    }
    else {
        // fallback textual hint for Back, left-aligned
        sf::Text hint2("ENTER or ESC: Back", gFont, 18);
        hint2.setFillColor(sf::Color(180, 180, 180));
        hint2.setPosition(leftX, winSize.y - 60.f);
        window.draw(hint2);
    }

    // Mouse click handling (toggle audio + track clicks for back visually)
    static bool wasMouseDown = false;
    bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (mouseDown && !wasMouseDown) {
        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        if (audRect.contains((float)mPos.x, (float)mPos.y)) {
            Ui_SetAudioEnabled(!Ui_AudioEnabled());
        }
        else {
            sf::FloatRect swRect(swBg.getPosition().x, swBg.getPosition().y, swW, swH);
            if (swRect.contains((float)mPos.x, (float)mPos.y)) {
                Ui_SetAudioEnabled(!Ui_AudioEnabled());
            }
            // NOTE: Back button click handling should be wired to your menu state manager.
            // We only expose the back button rect here for the caller to use.
            // Example: if (rOpt_Back.contains((float)mPos.x, (float)mPos.y)) -> request-pop-options
        }
    }
    wasMouseDown = mouseDown;
    // caller handles window.display()
}

// ----------------- PROFILE MENUS -----------------
void Gfx_DrawProfileMenu(sf::RenderWindow& window) {
    drawVignette(window);

    float w = 360.f, h = 100.f, gap = 30.f;
    float cx = window.getSize().x / 2.f - w / 2.f;
    float startY = (window.getSize().y - (3 * h + 2 * gap)) / 2.f;

    rProNew = { cx, startY, w, h };
    rProChoose = { cx, startY + h + gap, w, h };
    rProBack = { cx, startY + (h + gap) * 2, w, h };

    auto mouse = sf::Mouse::getPosition(window);
    sf::Vector2f m((float)mouse.x, (float)mouse.y);

    if (gProfileButtonImagesLoaded) {
        float baseScale = 0.5f;

        bool hoverNew = rProNew.contains(m);
        float newScale = baseScale * (hoverNew ? 1.05f : 1.0f);
        sf::Vector2u newSize = gBtnNewSaveTexture.getSize();
        float newW = newSize.x * newScale;
        float newH = newSize.y * newScale;
        float newX = cx + (w - newW) * 0.5f;
        float newY = startY + (h - newH) * 0.5f;
        gBtnNewSaveSprite.setScale(newScale, newScale);
        gBtnNewSaveSprite.setPosition(newX, newY);
        gBtnNewSaveSprite.setColor(hoverNew ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnNewSaveSprite);

        bool hoverChoose = rProChoose.contains(m);
        float chooseScale = baseScale * (hoverChoose ? 1.05f : 1.0f);
        sf::Vector2u chooseSize = gBtnLoadSaveTexture.getSize();
        float chooseW = chooseSize.x * chooseScale;
        float chooseH = chooseSize.y * chooseScale;
        float chooseX = cx + (w - chooseW) * 0.5f;
        float chooseY = startY + h + gap + (h - chooseH) * 0.5f;
        gBtnLoadSaveSprite.setScale(chooseScale, chooseScale);
        gBtnLoadSaveSprite.setPosition(chooseX, chooseY);
        gBtnLoadSaveSprite.setColor(hoverChoose ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnLoadSaveSprite);

        bool hoverBack = rProBack.contains(m);
        float backScale = baseScale * (hoverBack ? 1.05f : 1.0f);
        sf::Vector2u backSize = gBtnBackTexture.getSize();
        float backW = backSize.x * backScale;
        float backH = backSize.y * backScale;
        float backX = cx + (w - backW) * 0.5f;
        float backY = startY + (h + gap) * 2 + (h - backH) * 0.5f;
        gBtnBackSprite.setScale(backScale, backScale);
        gBtnBackSprite.setPosition(backX, backY);
        gBtnBackSprite.setColor(hoverBack ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnBackSprite);
    }
    else {
        // Fallback: draw pretty buttons if images are not loaded
        drawPrettyButton(window, rProNew, "New Save", hProNew);
        drawPrettyButton(window, rProChoose, "Load Save", hProChoose);
        drawPrettyButton(window, rProBack, "Back", hProBack);
    }
}

void Gfx_DrawProfileSelect(sf::RenderWindow& window, const std::vector<std::string>& list, int indexSelected)
{
    drawVignette(window);

    // Draw SaveSlots image if available
    if (gSaveSlotsLoaded) {
        sf::Vector2u imgSize = gSaveSlotsTexture.getSize();
        float scale = std::min(window.getSize().x / (float)imgSize.x * 0.6f,
            window.getSize().y / (float)imgSize.y * 0.15f);
        gSaveSlotsSprite.setScale(scale, scale);
        float posX = (window.getSize().x - imgSize.x * scale) * 0.5f;
        gSaveSlotsSprite.setPosition(posX, 30.f);
        window.draw(gSaveSlotsSprite);
    }
    else {
        // Fallback title if image not present
        sf::Text title("SAVE SLOTS", gFont, 40);
        title.setFillColor(sf::Color(100, 200, 255));
        sf::FloatRect b = title.getLocalBounds();
        title.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        title.setPosition(window.getSize().x / 2.f, 80.f);
        window.draw(title);
    }

    // Local layout constants (kept local so function doesn't depend on other translation-unit-only symbols)
    const float ITEM_H = 70.f;
    const float ITEM_W = 500.f;
    const float LIST_Y = 160.f;

    float listX = (window.getSize().x - ITEM_W) / 2.f;

    if (list.empty()) {
        sf::Text t("No profiles found!", gFont, 24);
        t.setPosition(listX + 150.f, 200.f);
        window.draw(t);
    }
    else {
        sf::Vector2f m = (sf::Vector2f)sf::Mouse::getPosition(window);

        for (size_t i = 0; i < list.size(); i++) {
            float y = LIST_Y + i * (ITEM_H + 15.f);
            sf::FloatRect r(listX, y, ITEM_W, ITEM_H);
            bool hover = r.contains(m);

            sf::Color fill = hover ? sf::Color(60, 70, 80) : sf::Color(30, 35, 40);
            sf::Color outline = hover ? sf::Color(100, 200, 255) : sf::Color(60, 60, 60);
            drawRoundBox(window, r, 8.f, fill, outline, 2.f);

            sf::Text tName(list[i], gFont, 26);
            tName.setStyle(sf::Text::Bold);
            tName.setPosition(listX + 20.f, y + 10.f);
            window.draw(tName);

            std::string info = Game_GetProfileInfo(list[i]);
            sf::Text tInfo(info, gFont, 18);
            tInfo.setFillColor(sf::Color(180, 180, 180));
            tInfo.setPosition(listX + 20.f, y + 42.f);
            window.draw(tInfo);

            // Highlight selection indexSelected if requested
            if ((int)i == indexSelected) {
                sf::RectangleShape sel({ ITEM_W - 4.f, ITEM_H - 4.f });
                sel.setPosition(listX + 2.f, y + 2.f);
                sel.setFillColor(sf::Color::Transparent);
                sel.setOutlineThickness(2.f);
                sel.setOutlineColor(sf::Color(220, 220, 80));
                window.draw(sel);
            }
        }
    }

    // Replace the guide positioning with centered alignment
    sf::Text guide("Click to Play | ESC to Back", gFont, 20);
    guide.setFillColor(sf::Color(200, 200, 200));
    sf::FloatRect gb = guide.getLocalBounds();
    guide.setOrigin(gb.left + gb.width / 2.f, gb.top + gb.height / 2.f);
    guide.setPosition(std::round(window.getSize().x * 0.5f), window.getSize().y - 50.f);
    window.draw(guide);
}

// ----------------- HIT TESTS & MISC DRAW -----------------

int Gfx_ProfileMenuHitTest(sf::Vector2f m) {
    if (rProNew.contains(m)) return 1;
    if (rProChoose.contains(m)) return 2;
    if (rProBack.contains(m)) return 3;
    return 0;
}

int Gfx_ProfileSelectHitTest(sf::Vector2f m, int listSize) {
    const float ITEM_H = 70.f;
    const float ITEM_W = 500.f;
    const float LIST_Y = 160.f;

    // Use board pixel width (matches other layouts that use BOARD_W)
    float realW = (float)(BOARD_W + BORDER * 2) * TILE;
    float listX = (realW - ITEM_W) / 2.f;

    for (int i = 0; i < listSize; i++) {
        float y = LIST_Y + i * (ITEM_H + 15.f);
        sf::FloatRect r(listX, y, ITEM_W, ITEM_H);
        if (r.contains(m)) return i;
    }
    return -1;
}

int Gfx_OptionsHitTest(sf::Vector2f m)
{
    // rOpt_Back is updated each frame in Gfx_DrawOptions when the Back sprite is drawn.
    // Return 1 when the Back control was clicked.
    if (rOpt_Back.contains(m)) return 1;
    return 0;
}

void Gfx_DrawNameInput(sf::RenderWindow& window, const std::string& currentInput) {
    drawVignette(window);

    if (gEnterNameLoaded) {
        sf::Vector2u tsize = gEnterNameTexture.getSize();
        float scale = std::min(window.getSize().x / (float)tsize.x * 0.6f, window.getSize().y / (float)tsize.y * 0.12f);
        gEnterNameSprite.setScale(scale, scale);
        gEnterNameSprite.setPosition((window.getSize().x - tsize.x * scale) * 0.5f, 60.f);
        window.draw(gEnterNameSprite);
    }
    else {
        sf::Text title("ENTER YOUR NAME", gFont, 40);
        sf::FloatRect b = title.getLocalBounds();
        title.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        title.setPosition(window.getSize().x / 2.f, 120.f);
        window.draw(title);
    }

    sf::FloatRect boxR(window.getSize().x / 2.f - 200.f, 240.f, 400.f, 60.f);
    drawRoundBox(window, boxR, 10.f, sf::Color(10, 10, 10), sf::Color::White, 2.f);

    sf::Text txt(currentInput + (time(NULL) % 2 == 0 ? "_" : ""), gFont, 32);
    sf::FloatRect tb = txt.getLocalBounds();
    txt.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    txt.setPosition(window.getSize().x / 2.f, boxR.top + boxR.height / 2.f);
    window.draw(txt);

    sf::Text hint("Press ENTER to Create | ESC to Cancel", gFont, 20);
    hint.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect hb = hint.getLocalBounds();
    hint.setOrigin(hb.left + hb.width / 2.f, hb.top + hb.height / 2.f);
    hint.setPosition(window.getSize().x / 2.f, boxR.top + boxR.height + 40.f);
    window.draw(hint);
}

void Gfx_DrawGameOverOverlay(sf::RenderWindow& window) {
    const float TITLE_SHRINK = 0.6f; // reduce PNG title size to 60% of calculated fit

    // Draw full-screen background for Game Over (prefer GameOverScreen.png, fall back to title or dark overlay)
    if (gGameOverBgLoaded) {
        sf::Vector2u bgSize = gGameOverBgTexture.getSize();
        float scaleX = (float)window.getSize().x / bgSize.x;
        float scaleY = (float)window.getSize().y / bgSize.y;
        float scale = std::max(scaleX, scaleY);
        gGameOverBgSprite.setScale(scale, scale);
        float posX = (window.getSize().x - bgSize.x * scale) * 0.5f;
        float posY = (window.getSize().y - bgSize.y * scale) * 0.5f;
        gGameOverBgSprite.setPosition(posX, posY);
        window.draw(gGameOverBgSprite);
    }
    else if (gGameOverTitleLoaded) {
        // stylized backdrop using title image; shrink it so text inside PNG appears smaller
        sf::Vector2u ts = gGameOverTitleTexture.getSize();
        float scaleX = (float)window.getSize().x / ts.x;
        float scaleY = (float)window.getSize().y / ts.y;
        float scale = std::max(scaleX, scaleY) * TITLE_SHRINK;
        gGameOverTitleSprite.setScale(scale, scale);
        float posX = (window.getSize().x - ts.x * scale) * 0.5f;
        float posY = (window.getSize().y - ts.y * scale) * 0.5f;
        gGameOverTitleSprite.setPosition(posX, posY);
        sf::Color prevCol = gGameOverTitleSprite.getColor();
        gGameOverTitleSprite.setColor(sf::Color(prevCol.r, prevCol.g, prevCol.b, 220));
        window.draw(gGameOverTitleSprite);
        gGameOverTitleSprite.setColor(prevCol);
    }
    else {
        sf::RectangleShape fov({ (float)window.getSize().x, (float)window.getSize().y });
        fov.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(fov);
    }

    // Title (draw on top of background). Shrink title sprite similarly so PNG text is visually smaller.
    if (gGameOverTitleLoaded) {
        sf::Vector2u ts = gGameOverTitleTexture.getSize();
        float scale = std::min(window.getSize().x / (float)ts.x * 0.6f, window.getSize().y / (float)ts.y * 0.18f);
        scale *= TITLE_SHRINK;
        gGameOverTitleSprite.setScale(scale, scale);
        gGameOverTitleSprite.setPosition((window.getSize().x - ts.x * scale) * 0.5f, window.getSize().y * 0.12f);
        window.draw(gGameOverTitleSprite);
    }
    else {
        // fallback smaller text
        sf::Text t("GAME OVER", gFont, 36); // reduced from 48
        t.setFillColor(sf::Color::Red);
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        t.setPosition(window.getSize().x * 0.5f, window.getSize().y * 0.18f);
        window.draw(t);
    }

    auto mouse = sf::Mouse::getPosition(window);
    sf::Vector2f m((float)mouse.x, (float)mouse.y);

    if (gGameOverBtnLoaded) {
        bool hoverRestart = rGO_Restart.contains(m);
        bool hoverExit = rGO_Exit.contains(m);

        const float BUTTON_SHRINK = 0.9f;

        // Restart button: scale to fit inside rGO_Restart and then apply hover boost
        if (gBtnRestart1Sprite.getTexture()) {
            sf::Vector2u tsr = gBtnRestart1Texture.getSize();
            if (tsr.x > 0 && tsr.y > 0) {
                float fitX = rGO_Restart.width / (float)tsr.x;
                float fitY = rGO_Restart.height / (float)tsr.y;
                float fit = std::min(fitX, fitY) * BUTTON_SHRINK;
                float actual = fit * (hoverRestart ? 1.06f : 1.0f);
                gBtnRestart1Sprite.setScale(actual, actual);
                float w = tsr.x * actual;
                float h = tsr.y * actual;
                gBtnRestart1Sprite.setPosition(
                    rGO_Restart.left + (rGO_Restart.width - w) * 0.5f,
                    rGO_Restart.top + (rGO_Restart.height - h) * 0.5f
                );
                gBtnRestart1Sprite.setColor(hoverRestart ? sf::Color::White : sf::Color(240, 240, 240));
                window.draw(gBtnRestart1Sprite);
            }
        }
        else {
            drawPrettyButton(window, rGO_Restart, "Restart", hGO_Restart);
        }

        // Exit button: scale to fit inside rGO_Exit and then apply hover boost
        if (gBtnExit1Sprite.getTexture()) {
            sf::Vector2u tse = gBtnExit1Texture.getSize();
            if (tse.x > 0 && tse.y > 0) {
                float fitX = rGO_Exit.width / (float)tse.x;
                float fitY = rGO_Exit.height / (float)tse.y;
                float fit = std::min(fitX, fitY) * BUTTON_SHRINK * 0.5f;
                float actual = fit * (hoverExit ? 1.06f : 1.0f);
                gBtnExit1Sprite.setScale(actual, actual);
                float w = tse.x * actual;
                float h = tse.y * actual;
                gBtnExit1Sprite.setPosition(
                    rGO_Exit.left + (rGO_Exit.width - w) * 0.5f,
                    rGO_Exit.top + (rGO_Exit.height - h) * 0.5f
                );
                gBtnExit1Sprite.setColor(hoverExit ? sf::Color::White : sf::Color(240, 240, 240));
                window.draw(gBtnExit1Sprite);
            }
        }
        else {
            drawPrettyButton(window, rGO_Exit, "Exit", hGO_Exit);
        }
    }
    else {
        drawPrettyButton(window, rGO_Restart, "Restart", hGO_Restart);
        drawPrettyButton(window, rGO_Exit, "Exit", hGO_Exit);
    }
}

int Gfx_GameOverHitTest(sf::Vector2f m) {
    if (rGO_Restart.contains(m)) return 1;
    if (rGO_Exit.contains(m)) return 2;
    return 0;
}

void Gfx_DrawPauseMenu(sf::RenderWindow& window) {
    const float TITLE_SHRINK = 0.6f; // reduce PNG title size to 60% of calculated fit

    if (gPauseScreenBgLoaded) {
        sf::Vector2u bgSize = gPauseScreenBgTexture.getSize();
        float scaleX = (float)window.getSize().x / bgSize.x;
        float scaleY = (float)window.getSize().y / bgSize.y;
        float scale = std::max(scaleX, scaleY);
        gPauseScreenBgSprite.setScale(scale, scale);
        float posX = (window.getSize().x - bgSize.x * scale) * 0.5f;
        float posY = (window.getSize().y - bgSize.y * scale) * 0.5f;
        gPauseScreenBgSprite.setPosition(posX, posY);
        window.draw(gPauseScreenBgSprite);
    }
    else {
        drawVignette(window);
    }

    // Title: shrink PNG title if present
    if (gPauseTitleLoaded) {
        sf::Vector2u ts = gPauseTitleTexture.getSize();
        float scale = std::min(window.getSize().x / (float)ts.x * 0.6f, window.getSize().y / (float)ts.y * 0.18f);
        scale *= TITLE_SHRINK;
        gPauseTitleSprite.setScale(scale, scale);
        gPauseTitleSprite.setPosition((window.getSize().x - ts.x * scale) * 0.5f, window.getSize().y * 0.12f);
        window.draw(gPauseTitleSprite);
    }
    else {
        // fallback smaller text
        sf::Text t("PAUSED", gFont, 36); // reduced from 46
        t.setFillColor(sf::Color(240, 240, 240));
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        t.setPosition(window.getSize().x * 0.5f, window.getSize().y * 0.16f);
        window.draw(t);
    }

    auto mouse = sf::Mouse::getPosition(window);
    sf::Vector2f m((float)mouse.x, (float)mouse.y);

    // Draw pause buttons: prefer image sprites; fallback uses drawPrettyButton (unchanged).
    if (gPauseButtonImagesLoaded) {
        const float BUTTON_SHRINK = 0.9f;

        // Resume
        bool hoverResume = rP_Resume.contains(m);
        if (gBtnResumeSprite.getTexture()) {
            sf::Vector2u ts = gBtnResumeTexture.getSize();
            if (ts.x > 0 && ts.y > 0) {
                float fitX = rP_Resume.width / (float)ts.x;
                float fitY = rP_Resume.height / (float)ts.y;
                float fit = std::min(fitX, fitY) * BUTTON_SHRINK;
                float actual = fit * (hoverResume ? 1.06f : 1.0f);
                gBtnResumeSprite.setScale(actual, actual);
                float w = ts.x * actual;
                float h = ts.y * actual;
                gBtnResumeSprite.setPosition(
                    rP_Resume.left + (rP_Resume.width - w) * 0.5f,
                    rP_Resume.top + (rP_Resume.height - h) * 0.5f
                );
                gBtnResumeSprite.setColor(hoverResume ? sf::Color::White : sf::Color(240, 240, 240));
                window.draw(gBtnResumeSprite);
            }
        }
        else {
            drawPrettyButton(window, rP_Resume, "Resume", hP_Resume);
        }

        // Restart
        bool hoverRestart = rP_Restart.contains(m);
        if (gBtnRestartSprite.getTexture()) {
            sf::Vector2u ts = gBtnRestartTexture.getSize();
            if (ts.x > 0 && ts.y > 0) {
                float fitX = rP_Restart.width / (float)ts.x;
                float fitY = rP_Restart.height / (float)ts.y;
                float fit = std::min(fitX, fitY) * BUTTON_SHRINK;
                float actual = fit * (hoverRestart ? 1.06f : 1.0f);
                gBtnRestartSprite.setScale(actual, actual);
                float w = ts.x * actual;
                float h = ts.y * actual;
                gBtnRestartSprite.setPosition(
                    rP_Restart.left + (rP_Restart.width - w) * 0.5f,
                    rP_Restart.top + (rP_Restart.height - h) * 0.5f
                );
                gBtnRestartSprite.setColor(hoverRestart ? sf::Color::White : sf::Color(240, 240, 240));
                window.draw(gBtnRestartSprite);
            }
        }
        else {
            drawPrettyButton(window, rP_Restart, "Restart", hP_Restart);
        }

        // Save
        bool hoverSave = rP_Save.contains(m);
        if (gBtnSaveSprite.getTexture()) {
            sf::Vector2u ts = gBtnSaveTexture.getSize();
            if (ts.x > 0 && ts.y > 0) {
                float fitX = rP_Save.width / (float)ts.x;
                float fitY = rP_Save.height / (float)ts.y;
                float fit = std::min(fitX, fitY) * BUTTON_SHRINK;
                float actual = fit * (hoverSave ? 1.06f : 1.0f);
                gBtnSaveSprite.setScale(actual, actual);
                float w = ts.x * actual;
                float h = ts.y * actual;
                gBtnSaveSprite.setPosition(
                    rP_Save.left + (rP_Save.width - w) * 0.5f,
                    rP_Save.top + (rP_Save.height - h) * 0.5f
                );
                gBtnSaveSprite.setColor(hoverSave ? sf::Color::White : sf::Color(240, 240, 240));
                window.draw(gBtnSaveSprite);
            }
        }
        else {
            drawPrettyButton(window, rP_Save, "Save Game", hP_Save);
        }

        // Exit
        bool hoverExit = rP_Exit.contains(m);
        if (gBtnExitSprite.getTexture()) {
            sf::Vector2u ts = gBtnExitTexture.getSize();
            if (ts.x > 0 && ts.y > 0) {
                float fitX = rP_Exit.width / (float)ts.x;
                float fitY = rP_Exit.height / (float)ts.y;
                float fit = std::min(fitX, fitY) * BUTTON_SHRINK;
                float actual = fit * (hoverExit ? 1.06f : 1.0f);
                gBtnExitSprite.setScale(actual, actual);
                float w = ts.x * actual;
                float h = ts.y * actual;
                gBtnExitSprite.setPosition(
                    rP_Exit.left + (rP_Exit.width - w) * 0.5f,
                    rP_Exit.top + (rP_Exit.height - h) * 0.5f
                );
                gBtnExitSprite.setColor(hoverExit ? sf::Color::White : sf::Color(240, 240, 240));
                window.draw(gBtnExitSprite);
            }
        }
        else {
            drawPrettyButton(window, rP_Exit, "Exit", hP_Exit);
        }
    }
    else {
        drawPrettyButton(window, rP_Resume, "Resume", hP_Resume);
        drawPrettyButton(window, rP_Restart, "Restart", hP_Restart);
        drawPrettyButton(window, rP_Save, "Save Game", hP_Save);
        drawPrettyButton(window, rP_Exit, "Exit", hP_Exit);
    }
}

MenuHit Gfx_MenuHitTest(const sf::Vector2f& m) {
    if (!Game_Paused()) return MH_None;
    if (rP_Resume.contains(m)) return MH_Resume;
    if (rP_Restart.contains(m)) return MH_Restart;
    if (rP_Save.contains(m)) return MH_Save;
    if (rP_Load.contains(m)) return MH_Load;
    if (rP_Exit.contains(m)) return MH_Exit;
    return MH_None;
}