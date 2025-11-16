#include "gfx.h"
#include "config.h"
#include "game.h"
#include <string>
#include <algorithm>
#include <cmath>
#include <iostream>

// ================== Biến/Tài nguyên đồ hoạ cốt lõi ==================
static sf::Font gFont;
static sf::Text gHud, gSeg;

static sf::RectangleShape gHeadRect, gBodyRect;
static sf::CircleShape    gFoodShape;
static sf::RectangleShape gObsRect;

static sf::CircleShape    gPortalAShape, gPortalBShape;

// === THÊM TEXTURE VÀ SPRITE CHO BACKGROUND VÀ UI ELEMENTS ===
static sf::Texture gMainMenuBgTexture;
static sf::Sprite gMainMenuBgSprite;
static bool gMainMenuBgLoaded = false;

// Title image (1 hình duy nhất cho "SNAKE GAME")
static sf::Texture gTitleTexture;
static sf::Sprite gTitleSprite;
static bool gTitleImageLoaded = false;

// Button images
static sf::Texture gBtnStartTexture, gBtnQuitTexture;
static sf::Sprite gBtnStartSprite, gBtnQuitSprite;

static sf::Texture gBtnOptionTexture;
static sf::Sprite gBtnOptionSprite;

static bool gButtonImagesLoaded = false;

// Âm thanh (optional – nếu không có file vẫn chạy)
static sf::SoundBuffer gEatBuf, gDieBuf;
static sf::Sound gEat, gDie;

// ===== Pause Menu =====
static sf::RectangleShape gMenuPanel;
static sf::RectangleShape gBtnResume, gBtnRestart, gBtnExit;
static sf::Text gTxtResume, gTxtRestart, gTxtExit;
static int gHover = 0; // 0=none,1=Resume,2=Restart,3=Exit

// ===== Main Menu Buttons =====
static sf::FloatRect gBtnPlayR, gBtnOptR, gBtnQuitR;

// ===== Helpers =====
static inline sf::Vector2f CellToPx(int cx, int cy) {
    return sf::Vector2f((cx + BORDER) * TILE, (cy + BORDER) * TILE);
}

static bool InRect(const sf::RectangleShape& r, const sf::Vector2f& p) {
    auto a = r.getPosition(), s = r.getSize();
    return (p.x >= a.x && p.x <= a.x + s.x && p.y >= a.y && p.y <= a.y + s.y);
}

static void LayoutPauseMenu(int winW, int winH) {
    const float panelW = 360.f, panelH = 240.f;
    const float px = (winW - panelW) / 2.f, py = (winH - panelH) / 2.f;

    gMenuPanel.setPosition(px, py);
    gMenuPanel.setSize({ panelW,panelH });
    gMenuPanel.setFillColor(sf::Color(25, 25, 25, 240));
    gMenuPanel.setOutlineThickness(2.f);
    gMenuPanel.setOutlineColor(sf::Color(90, 90, 90));

    const float bw = panelW - 60.f, bh = 48.f;
    const float bx = px + 30.f;
    float by = py + 60.f;

    auto prepBtn = [&](sf::RectangleShape& r, sf::Text& t, const char* label) {
        r.setSize({ bw,bh });
        r.setPosition(bx, by);
        r.setFillColor(sf::Color(50, 50, 50));
        r.setOutlineThickness(2.f);
        r.setOutlineColor(sf::Color(80, 80, 80));

        t.setFont(gFont);
        t.setString(label);
        t.setCharacterSize(22);
        t.setFillColor(sf::Color(230, 230, 230));
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        t.setPosition(bx + bw / 2.f, by + bh / 2.f);

        by += bh + 18.f;
        };
    prepBtn(gBtnResume, gTxtResume, "Resume (P)");
    prepBtn(gBtnRestart, gTxtRestart, "Restart (Space)");
    prepBtn(gBtnExit, gTxtExit, "Exit");
}

void Gfx_MenuUpdateHover(const sf::Vector2f& m) {
    if (!Game_Paused()) { gHover = 0; return; }
    gHover = 0;
    if (InRect(gBtnResume, m)) gHover = 1;
    if (InRect(gBtnRestart, m)) gHover = 2;
    if (InRect(gBtnExit, m)) gHover = 3;

    auto col = [&](int id) { return (gHover == id) ? sf::Color(70, 110, 180) : sf::Color(50, 50, 50); };
    gBtnResume.setFillColor(col(1));
    gBtnRestart.setFillColor(col(2));
    gBtnExit.setFillColor(col(3));
}

MenuHit Gfx_MenuHitTest(const sf::Vector2f& m) {
    if (!Game_Paused()) return MH_None;
    if (InRect(gBtnResume, m)) return MH_Resume;
    if (InRect(gBtnRestart, m)) return MH_Restart;
    if (InRect(gBtnExit, m)) return MH_Exit;
    return MH_None;
}

// ================== Snake Skin ==================
static int gSkin = 0; // 0..5

static sf::Color Lerp(const sf::Color& a, const sf::Color& b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        (sf::Uint8)(a.r + (b.r - a.r) * t),
        (sf::Uint8)(a.g + (b.g - a.g) * t),
        (sf::Uint8)(a.b + (b.b - a.b) * t),
        (sf::Uint8)(a.a + (b.a - a.a) * t)
    );
}

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

static sf::Color SkinColorFor(std::size_t i, std::size_t len, bool head) {
    switch (gSkin) {
    case 0: return head ? sf::Color(0, 180, 120) : sf::Color(0, 140, 90); // Classic
    case 1: { // Neon
        if (head) return sf::Color(0, 230, 200);
        float t = (std::sin((float)i * 0.8f) + 1.f) * 0.5f;
        return Lerp(sf::Color(0, 140, 160), sf::Color(0, 200, 190), t);
    }
    case 2: { // Rainbow
        float h = 360.f * (float)i / std::max<std::size_t>(1, len);
        return FromHSV(h, 0.85f, head ? 1.f : 0.9f);
    }
    case 3: { // Stripes
        static sf::Color C[3] = { sf::Color(40,180,110), sf::Color(80,170,220), sf::Color(240,180,60) };
        return head ? sf::Color(255, 255, 255) : C[i % 3];
    }
    case 4: { // Gradient
        float t = (float)i / std::max<std::size_t>(1, len - 1);
        return Lerp(sf::Color(0, 170, 110), sf::Color(230, 210, 60), t);
    }
    case 5: return head ? sf::Color(255, 208, 70) : sf::Color(200, 150, 40); // Gold
    default: return head ? sf::Color(0, 180, 120) : sf::Color(0, 140, 90);
    }
}

void Gfx_SetSkin(int id) { gSkin = std::clamp(id, 0, 5); }
int  Gfx_CurrentSkin() { return gSkin; }
const char* Gfx_SkinName() {
    static const char* N[] = { "Classic","Neon","Rainbow","Stripes","Grad","Gold" };
    return N[std::clamp(gSkin, 0, 5)];
}

// ================== Skin Picker Menu (F1) ==================
static bool  gSkinMenuOn = false;
static int   gSkinHover = -1;
static sf::FloatRect gSkinPanel;
static sf::FloatRect gSkinRect[6];
static const int SKIN_CNT = 6;
static sf::RectangleShape gUiRect;
static sf::Text gUiText;

static void LayoutSkinMenu(float W, float H) {
    float pw = 560.f, ph = 320.f;
    gSkinPanel = sf::FloatRect((W - pw) / 2.f, (H - ph) / 2.f, pw, ph);

    float m = 24.f, gap = 18.f;
    float cw = (pw - m * 2 - gap * 2) / 3.f;
    float ch = (ph - 90.f - gap) / 2.f;

    int k = 0;
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c, ++k) {
            gSkinRect[k] = sf::FloatRect(
                gSkinPanel.left + m + c * (cw + gap),
                gSkinPanel.top + 60.f + r * (ch + gap),
                cw, ch
            );
        }
}

void Gfx_SkinMenuToggle() { gSkinMenuOn = !gSkinMenuOn; }
bool Gfx_SkinMenuOn() { return gSkinMenuOn; }

void Gfx_SkinMenuOnEvent(const sf::Event& e) {
    if (!gSkinMenuOn) return;

    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::F1)
            gSkinMenuOn = false;
        if (e.key.code >= sf::Keyboard::Num1 && e.key.code <= sf::Keyboard::Num6) {
            Gfx_SetSkin(e.key.code - sf::Keyboard::Num1);
            gSkinMenuOn = false;
        }
    }
    if (e.type == sf::Event::MouseMoved) {
        sf::Vector2f m((float)e.mouseMove.x, (float)e.mouseMove.y);
        gSkinHover = -1;
        for (int i = 0; i < SKIN_CNT; ++i) if (gSkinRect[i].contains(m)) { gSkinHover = i; break; }
    }
    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
        if (gSkinHover >= 0) {
            Gfx_SetSkin(gSkinHover);
            gSkinMenuOn = false;
        }
    }
}

static void DrawSkinPreview(sf::RenderWindow& win, const sf::FloatRect& r, int id) {
    gUiRect.setPosition(r.left, r.top);
    gUiRect.setSize({ r.width, r.height });
    gUiRect.setFillColor(sf::Color(25, 25, 25, 220));
    win.draw(gUiRect);

    float tile = std::min(r.width / 6.f, r.height / 3.f);
    sf::RectangleShape seg({ tile - 3.f, tile - 3.f });
    for (int i = 0; i < 6; ++i) {
        bool head = (i == 0);
        seg.setFillColor(SkinColorFor((std::size_t)i, 6, head));
        seg.setPosition(r.left + 8.f + i * tile, r.top + r.height * 0.5f - tile * 0.5f);
        win.draw(seg);
    }

    sf::RectangleShape bdr({ r.width, r.height });
    bdr.setPosition(r.left, r.top);
    bdr.setFillColor(sf::Color::Transparent);
    bdr.setOutlineThickness((id == Gfx_CurrentSkin()) ? 4.f : 2.f);
    bdr.setOutlineColor((id == gSkinHover) ? sf::Color(90, 180, 255) :
        (id == Gfx_CurrentSkin()) ? sf::Color(250, 230, 80) :
        sf::Color(90, 90, 90));
    win.draw(bdr);

    static const char* NAME[] = { "Classic","Neon","Rainbow","Stripes","Grad","Gold" };
    gUiText.setString(NAME[id]);
    gUiText.setCharacterSize(16);
    sf::FloatRect T = gUiText.getLocalBounds();
    gUiText.setPosition(r.left + (r.width - T.width) / 2.f, r.top + r.height - T.height - 10.f);
    win.draw(gUiText);
}

void Gfx_SkinMenuDraw(sf::RenderWindow& window) {
    if (!gSkinMenuOn) return;

    auto sz = window.getSize();
    LayoutSkinMenu((float)sz.x, (float)sz.y);

    sf::RectangleShape dim({ (float)sz.x,(float)sz.y });
    dim.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(dim);

    gUiRect.setPosition(gSkinPanel.left, gSkinPanel.top);
    gUiRect.setSize({ gSkinPanel.width, gSkinPanel.height });
    gUiRect.setFillColor(sf::Color(30, 30, 30, 235));
    window.draw(gUiRect);

    gUiText.setString("Select Skin (click) — F1/Esc to close");
    gUiText.setCharacterSize(24);
    sf::FloatRect t = gUiText.getLocalBounds();
    gUiText.setPosition(gSkinPanel.left + (gSkinPanel.width - t.width) / 2.f, gSkinPanel.top + 16.f);
    window.draw(gUiText);

    for (int i = 0; i < SKIN_CNT; ++i) DrawSkinPreview(window, gSkinRect[i], i);
}

// ================== Khởi tạo ==================
bool Gfx_Init(sf::RenderWindow& window) {
    window.setVerticalSyncEnabled(true);

    if (!gFont.loadFromFile("assets/fonts/RobotoMono-Regular.ttf")) {
        std::cerr << "Failed to load font assets/fonts/RobotoMono-Regular.ttf\n";
    }

    if (gMainMenuBgTexture.loadFromFile("assets/UI/MainScreen.png")) {
        gMainMenuBgSprite.setTexture(gMainMenuBgTexture);
        gMainMenuBgLoaded = true;
        std::cout << "Main menu background loaded successfully!\n";
    }
    else {
        std::cerr << "Warning: Failed to load assets/UI/MainScreen.png\n";
        gMainMenuBgLoaded = false;
    }

    if (gTitleTexture.loadFromFile("assets/UI/SnakeGame.png")) {
        gTitleSprite.setTexture(gTitleTexture);
        gTitleImageLoaded = true;
        std::cout << "Title 'SNAKE GAME' image loaded successfully!\n";
    }
    else {
        std::cerr << "Warning: Failed to load assets/UI/SnakeGame.png\n";
        gTitleImageLoaded = false;
    }

    if (gBtnStartTexture.loadFromFile("assets/UI/Start.png")) {
        gBtnStartSprite.setTexture(gBtnStartTexture);
        std::cout << "Start button image loaded successfully!\n";
    }
    else {
        std::cerr << "Warning: Failed to load assets/UI/Start.png\n";
    }

    if (gBtnOptionTexture.loadFromFile("assets/UI/Option.png")) {
        gBtnOptionSprite.setTexture(gBtnOptionTexture);
        std::cout << "Option button image loaded successfully!\n";
    }
    else {
        std::cerr << "Warning: Failed to load assets/UI/Option.png\n";
    }

    if (gBtnQuitTexture.loadFromFile("assets/UI/Quit.png")) {
        gBtnQuitSprite.setTexture(gBtnQuitTexture);
        std::cout << "Quit button image loaded successfully!\n";
    }
    else {
        std::cerr << "Warning: Failed to load assets/UI/Quit.png\n";
    }

    gButtonImagesLoaded = (gBtnStartTexture.getSize().x > 0 && gBtnOptionTexture.getSize().x > 0 && gBtnQuitTexture.getSize().x > 0);

    gHud.setFont(gFont); gHud.setCharacterSize(18); gHud.setFillColor(sf::Color(220, 220, 220));
    gSeg.setFont(gFont); gSeg.setCharacterSize(TILE - 10); gSeg.setFillColor(sf::Color::White);

    gHeadRect.setSize({ (float)TILE - 2,(float)TILE - 2 }); gHeadRect.setFillColor(sf::Color(0, 180, 120));
    gBodyRect.setSize({ (float)TILE - 2,(float)TILE - 2 }); gBodyRect.setFillColor(sf::Color(0, 140, 90));

    gFoodShape = sf::CircleShape(TILE * 0.35f);
    gFoodShape.setFillColor(sf::Color(220, 160, 40));

    gObsRect.setSize({ (float)TILE - 2,(float)TILE - 2 });
    gObsRect.setFillColor(sf::Color(170, 70, 70));

    gPortalAShape = sf::CircleShape(TILE * 0.45f); gPortalAShape.setFillColor(sf::Color(80, 120, 220));
    gPortalBShape = sf::CircleShape(TILE * 0.45f); gPortalBShape.setFillColor(sf::Color(200, 90, 200));

    gEatBuf.loadFromFile("assets/sfx/eat.wav"); gEat.setBuffer(gEatBuf); gEat.setVolume(60.f);
    gDieBuf.loadFromFile("assets/sfx/die.wav"); gDie.setBuffer(gDieBuf); gDie.setVolume(70.f);

    LayoutPauseMenu(window.getSize().x, window.getSize().y);

    gUiText.setFont(gFont);
    gUiText.setFillColor(sf::Color::White);
    gUiText.setCharacterSize(18);
    gUiRect.setFillColor(sf::Color(30, 30, 30, 230));

    return true;
}

// ================== Vẽ 1 frame ==================
void Gfx_DrawFrame(sf::RenderWindow& window) {
    const int WIN_W = (BOARD_W + BORDER * 2) * TILE;
    const int WIN_H = (BOARD_H + BORDER * 2) * TILE;

    int ev = Game_ConsumeEvents();
    if (ev & 1) gEat.play();
    if (ev & 2) gDie.play();

    window.clear(sf::Color(30, 30, 30));

    sf::RectangleShape r; r.setFillColor(sf::Color(60, 60, 60));
    r.setSize({ (float)WIN_W, (float)TILE }); r.setPosition(0, 0); window.draw(r);
    r.setPosition(0, WIN_H - TILE); window.draw(r);
    r.setSize({ (float)TILE, (float)WIN_H - TILE * 2 }); r.setPosition(0, TILE); window.draw(r);
    r.setPosition(WIN_W - TILE, TILE); window.draw(r);

    sf::Vertex line[2]; sf::Color gridCol(50, 50, 50);
    for (int y = 0; y <= BOARD_H; ++y) {
        float py = (BORDER + y) * TILE;
        line[0] = { {(float)BORDER * TILE, py}, gridCol };
        line[1] = { {(float)WIN_W - BORDER * TILE, py}, gridCol };
        window.draw(line, 2, sf::Lines);
    }
    for (int x = 0; x <= BOARD_W; ++x) {
        float px = (BORDER + x) * TILE;
        line[0] = { {px,(float)BORDER * TILE}, gridCol };
        line[1] = { {px,(float)WIN_H - BORDER * TILE}, gridCol };
        window.draw(line, 2, sf::Lines);
    }

    Cell f = Game_Food();
    auto pf = CellToPx(f.x, f.y);
    gFoodShape.setPosition(pf.x + TILE * 0.15f, pf.y + TILE * 0.15f);
    window.draw(gFoodShape);

    for (std::size_t i = 0; i < Game_ObstacleCount(); ++i) {
        Cell o = Game_Obstacle(i);
        auto p = CellToPx(o.x, o.y);
        gObsRect.setPosition(p.x + 1, p.y + 1);
        window.draw(gObsRect);
    }

    if (Game_PortalsActive()) {
        Cell pa = Game_PortalA(), pb = Game_PortalB();
        auto pA = CellToPx(pa.x, pa.y);
        auto pB = CellToPx(pb.x, pb.y);
        gPortalAShape.setPosition(pA.x + TILE * 0.05f, pA.y + TILE * 0.05f);
        gPortalBShape.setPosition(pB.x + TILE * 0.05f, pB.y + TILE * 0.05f);
        window.draw(gPortalAShape);
        window.draw(gPortalBShape);
    }

    bool first = true;
    std::size_t len = Game_SnakeLen();
    for (std::size_t i = 0; i < len; ++i) {
        Cell c = Game_SnakeSeg(i);
        auto p = CellToPx(c.x, c.y);
        sf::Color col = SkinColorFor(i, len, first);
        if (first) {
            gHeadRect.setFillColor(col);
            gHeadRect.setPosition(p.x + 1, p.y + 1);
            window.draw(gHeadRect);
            first = false;
        }
        else {
            gBodyRect.setFillColor(col);
            gBodyRect.setPosition(p.x + 1, p.y + 1);
            window.draw(gBodyRect);
        }

        if (!std::string(MSSV_STRING).empty()) {
            char ch = MSSV_STRING[i % std::string(MSSV_STRING).size()];
            gSeg.setString(sf::String(ch));
            gSeg.setPosition(p.x + TILE * 0.28f, p.y + 2.f);
            window.draw(gSeg);
        }
    }

    gHud.setString(
        "Score: " + std::to_string(Game_Score()) +
        "   HS: " + std::to_string(Game_HighScore()) +
        "   Len: " + std::to_string((int)Game_SnakeLen()) +
        "   Lv: " + std::to_string(Game_Level()) +
        "   Wrap: " + std::string(Game_WrapOn() ? "ON" : "OFF") +
        "   Port: " + std::string(Game_PortalsActive() ? "ON" : "OFF") +
        "   Spd: " + std::to_string((int)(1.f / Game_MoveInterval())) + " step/s" +
        "   Skin: " + std::string(Gfx_SkinName())
    );
    gHud.setPosition(8.f, 4.f);
    window.draw(gHud);

    if (Game_Over()) {
        sf::RectangleShape fov({ (float)WIN_W,(float)WIN_H });
        fov.setFillColor(sf::Color(0, 0, 0, 120)); window.draw(fov);

        sf::Text big; big.setFont(gFont); big.setCharacterSize(40);
        big.setFillColor(sf::Color(240, 240, 240));
        big.setString("GAME OVER\nEnter to restart");
        sf::FloatRect b = big.getLocalBounds();
        big.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        big.setPosition(WIN_W / 2.f, WIN_H / 2.f);
        window.draw(big);
    }
    else if (Game_Paused()) {
        LayoutPauseMenu(WIN_W, WIN_H);

        sf::RectangleShape fov({ (float)WIN_W,(float)WIN_H });
        fov.setFillColor(sf::Color(0, 0, 0, 140)); window.draw(fov);

        window.draw(gMenuPanel);
        window.draw(gBtnResume);  window.draw(gTxtResume);
        window.draw(gBtnRestart); window.draw(gTxtRestart);
        window.draw(gBtnExit);    window.draw(gTxtExit);
    }

    Gfx_SkinMenuDraw(window);
    window.display();
}

// ================== Main Menu ==================
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

        gBtnPlayR = { (ww - btnW) * 0.5f, startY, btnW, btnH };
        gBtnOptR = { (ww - btnW) * 0.5f, startY + btnH + gap, btnW, btnH };
        gBtnQuitR = { (ww - quitW) * 0.5f, startY + (btnH + gap) * 2, quitW, quitH };
    }
    else {
        float panelW = std::min<float>(420.f, ww * 0.6f);
        float panelX = (ww - panelW) * 0.5f;
        float y = wh * 0.28f;
        float h = 56.f, gap = 18.f;

        gBtnPlayR = { panelX, y, panelW, h };
        gBtnOptR = { panelX, y + h + gap, panelW, h };
        gBtnQuitR = { panelX, y + (h + gap) * 2, panelW, h };
    }
}

const sf::FloatRect& Gfx_BtnPlay() { return gBtnPlayR; }
const sf::FloatRect& Gfx_BtnOptions() { return gBtnOptR; }
const sf::FloatRect& Gfx_BtnQuit() { return gBtnQuitR; }

void Gfx_DrawMainMenu(sf::RenderWindow& window, int previewSkin)
{
    sf::Vector2u winSize = window.getSize();

    // Vẽ background
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
    else {
        window.clear(sf::Color(25, 25, 25));
    }

    // Vẽ title
    if (gTitleImageLoaded) {
        float titleScale = std::min(winSize.x / 1000.f, winSize.y / 800.f);
        titleScale = std::max(0.3f, std::min(titleScale, 0.8f));
        sf::Vector2u titleSize = gTitleTexture.getSize();
        gTitleSprite.setScale(titleScale, titleScale);
        float titleW = titleSize.x * titleScale;
        gTitleSprite.setPosition((winSize.x - titleW) * 0.5f, winSize.y * 0.1f);
        window.draw(gTitleSprite);
    }
    else {
        sf::Text title("S N A K E  G A M E", gFont, 48);
        title.setFillColor(sf::Color(230, 230, 230));
        title.setOutlineThickness(3.f);
        title.setOutlineColor(sf::Color(20, 20, 20));
        sf::FloatRect b = title.getLocalBounds();
        title.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
        title.setPosition(winSize.x * 0.5f, winSize.y * 0.16f);
        window.draw(title);
    }

    // Vẽ buttons
    auto mouse = sf::Mouse::getPosition(window);
    sf::Vector2f m((float)mouse.x, (float)mouse.y);

    if (gButtonImagesLoaded) {
        bool hoverStart  = gBtnPlayR.contains(m);
        bool hoverOption = gBtnOptR.contains(m);
        bool hoverQuit   = gBtnQuitR.contains(m);

        float scale = std::min(winSize.x / 1200.f, winSize.y / 900.f);
        scale = std::max(0.35f, std::min(scale, 0.7f));

        // Start button
        float startScale = scale * (hoverStart ? 1.1f : 1.0f);
        gBtnStartSprite.setScale(startScale, startScale);
        sf::Vector2u startSize = gBtnStartTexture.getSize();
        float startW = startSize.x * startScale;
        gBtnStartSprite.setPosition((winSize.x - startW) * 0.5f, gBtnPlayR.top);
        gBtnStartSprite.setColor(hoverStart ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnStartSprite);

        // Options button
        float optionScale = scale * (hoverOption ? 1.1f : 1.0f);
        gBtnOptionSprite.setScale(optionScale, optionScale);
        sf::Vector2u optionSize = gBtnOptionTexture.getSize();
        float optionW = optionSize.x * optionScale;
        gBtnOptionSprite.setPosition((winSize.x - optionW) * 0.5f, gBtnOptR.top);
        gBtnOptionSprite.setColor(hoverOption ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnOptionSprite);

        // Quit button
        float quitScale = scale * (hoverQuit ? 1.1f : 1.0f);
        gBtnQuitSprite.setScale(quitScale, quitScale);
        sf::Vector2u quitSize = gBtnQuitTexture.getSize();
        float quitW = quitSize.x * quitScale;
        gBtnQuitSprite.setPosition((winSize.x - quitW) * 0.5f, gBtnQuitR.top);
        gBtnQuitSprite.setColor(hoverQuit ? sf::Color(255, 255, 255) : sf::Color(240, 240, 240));
        window.draw(gBtnQuitSprite);
    }
    else {
        auto hover = [&](const sf::FloatRect& r) { return r.contains(m); };
        drawButton(window, gBtnPlayR, "PLAY", hover(gBtnPlayR));
        drawButton(window, gBtnOptR, "OPTIONS", hover(gBtnOptR));
        drawButton(window, gBtnQuitR, "QUIT", hover(gBtnQuitR));
    }

    // Skin preview
    float cx = winSize.x * 0.5f, cy = winSize.y * 0.85f;
    sf::RectangleShape cell({ 22.f,22.f });
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
    float startX = cx - 60.f;
    for (int i = 0; i < 6; i++) {
        cell.setPosition(startX + i * 24.f, cy);
        cell.setFillColor(i == previewSkin ? sf::Color::White : colorFor(i));
        window.draw(cell);
    }

    sf::Text cap("Skin: 1..6  |  Options to change", gFont, 16);
    cap.setFillColor(sf::Color(230, 230, 230));
    cap.setOutlineThickness(1.5f);
    cap.setOutlineColor(sf::Color(10, 10, 10));
    cap.setPosition(cx - cap.getLocalBounds().width / 2.f, cy + 32.f);
    window.draw(cap);
}

void Gfx_DrawOptions(sf::RenderWindow& window, int previewSkin)
{
    window.clear(sf::Color(25, 25, 25));

    sf::Text title("OPTIONS", gFont, 42);
    title.setFillColor(sf::Color(230, 230, 230));
    sf::FloatRect b = title.getLocalBounds();
    title.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    title.setPosition(window.getSize().x * 0.5f, window.getSize().y * 0.16f);
    window.draw(title);

    sf::Text hint("Choose Snake Skin: 1..6  |  ENTER: Back  |  ESC: Back", gFont, 20);
    hint.setFillColor(sf::Color(200, 200, 200));
    hint.setPosition(40.f, window.getSize().y * 0.28f);
    window.draw(hint);

    float cx = window.getSize().x * 0.5f, cy = window.getSize().y * 0.58f;
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

    float startX = cx - 3 * 42.f;
    for (int i = 0; i < 6; i++) {
        cell.setPosition(startX + i * 42.f, cy);
        cell.setFillColor(colorFor(i));
        cell.setOutlineThickness(i == previewSkin ? 3.f : 1.f);
        cell.setOutlineColor(i == previewSkin ? sf::Color::White : sf::Color(80, 80, 80));
        window.draw(cell);
    }
}
