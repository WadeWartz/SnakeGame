#include <SFML/Graphics.hpp>
#include "config.h"
#include "game.h"
#include "game_render.h"   // <-- đổi gfx.h thành game_render.h
#include "ui_gfx.h"        // <-- thêm UI
#include <vector>
#include <string>
#include <iostream>
#include <windows.h> // add this so we can disable Windows UI scaling for the process

// Định nghĩa trạng thái game
enum AppState {
    ST_MENU,
    ST_OPTIONS,
    ST_PLAY,
    ST_PROFILE_MENU,
    ST_NEW_PROFILE,
    ST_SELECT_PROFILE
};

int main() {
    const int WIN_W = (BOARD_W + BORDER * 2) * TILE;
    const int WIN_H = (BOARD_H + BORDER * 2) * TILE;

    // Create window with MSAA disabled and force a 1:1 logical view
    sf::ContextSettings cs;
    cs.antialiasingLevel = 0;
    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H),
        "Snake RPG - Profile System",
        sf::Style::Titlebar | sf::Style::Close,
        cs
    );
    window.setVerticalSyncEnabled(true);
    // force a 1:1 view that matches the window pixel size (prevents DPI/view scaling blur)
    window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)WIN_W, (float)WIN_H)));

    // ==== INIT ====
    Game_Init();
    Game_LoadHighScore();

    Gfx_Init(window);           // gameplay init
    Ui_Init(window);            // UI init
    Gfx_MenuLayout(WIN_W, WIN_H);

    AppState state = ST_MENU;
    int previewSkin = Gfx_CurrentSkin();
    sf::Clock clock;

    std::string inputName = "";
    std::vector<std::string> profileList;

    // =========================================================
    // ======================= GAME LOOP ========================
    // =========================================================

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::Resized) {
                // Keep the view matching new pixel size to avoid sub-pixel scaling
                window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)e.size.width, (float)e.size.height)));
            }

            // ────────────────────────────────────────────
            // 1. MENU CHÍNH
            if (state == ST_MENU) {

                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::Enter) {
                        state = ST_PROFILE_MENU;
                    }
                }

                // Chuột
                if (e.type == sf::Event::MouseButtonPressed &&
                    e.mouseButton.button == sf::Mouse::Left) {

                    sf::Vector2f m(e.mouseButton.x, e.mouseButton.y);

                    if (Gfx_BtnPlay().contains(m)) {
                        state = ST_PROFILE_MENU;
                    }
                    else if (Gfx_BtnOptions().contains(m)) {
                        state = ST_OPTIONS;
                    }
                    else if (Gfx_BtnQuit().contains(m)) {
                        window.close();
                    }
                }
            }

            // ────────────────────────────────────────────
            // 2. PROFILE MENU
            else if (state == ST_PROFILE_MENU) {

                if (e.type == sf::Event::MouseButtonPressed &&
                    e.mouseButton.button == sf::Mouse::Left) {

                    sf::Vector2f m(e.mouseButton.x, e.mouseButton.y);
                    int action = Gfx_ProfileMenuHitTest(m);

                    if (action == 1) {
                        state = ST_NEW_PROFILE;
                        inputName = "";
                    }
                    else if (action == 2) {
                        profileList = Game_GetProfileList();
                        state = ST_SELECT_PROFILE;
                    }
                    else if (action == 3) {
                        state = ST_MENU;
                    }
                }
            }

            // ────────────────────────────────────────────
            // 3. NEW PROFILE
            else if (state == ST_NEW_PROFILE) {

                if (e.type == sf::Event::TextEntered) {

                    if (e.text.unicode == 8) { // backspace
                        if (!inputName.empty()) inputName.pop_back();
                    }
                    else if (e.text.unicode == 13) { // enter
                        if (!inputName.empty()) {
                            if (Game_CreateProfile(inputName)) {
                                Game_SetProfileName(inputName);
                                Game_Reset();
                                state = ST_PLAY;
                            }
                            else {
                                std::cout << "Tên đã tồn tại!\n";
                            }
                        }
                    }
                    else if (e.text.unicode < 128 && isprint(e.text.unicode)) {
                        if (inputName.length() < 12)
                            inputName += char(e.text.unicode);
                    }
                }

                if (e.type == sf::Event::KeyPressed &&
                    e.key.code == sf::Keyboard::Escape)
                    state = ST_PROFILE_MENU;
            }

            // ────────────────────────────────────────────
            // 4. SELECT PROFILE
            else if (state == ST_SELECT_PROFILE) {

                if (e.type == sf::Event::MouseButtonPressed &&
                    e.mouseButton.button == sf::Mouse::Left) {

                    sf::Vector2f m(e.mouseButton.x, e.mouseButton.y);
                    int idx = Gfx_ProfileSelectHitTest(m, (int)profileList.size());

                    if (idx != -1) {
                        std::string name = profileList[idx];
                        Game_SetProfileName(name);

                        if (!Game_LoadGame())
                            Game_Reset();

                        Game_SetPaused(false);
                        state = ST_PLAY;
                    }
                }

                if (e.type == sf::Event::KeyPressed &&
                    e.key.code == sf::Keyboard::Escape)
                    state = ST_PROFILE_MENU;
            }

            // ────────────────────────────────────────────
            // 5. OPTIONS
            else if (state == ST_OPTIONS) {

                if (e.type == sf::Event::KeyPressed) {

                    if (e.key.code >= sf::Keyboard::Num1 && e.key.code <= sf::Keyboard::Num6)
                        previewSkin = (int)e.key.code - (int)sf::Keyboard::Num1;

                    if (e.key.code == sf::Keyboard::Enter ||
                        e.key.code == sf::Keyboard::Escape) {
                        Gfx_SetSkin(previewSkin); // apply here only
                        state = ST_MENU;
                    }
                }

                if (e.type == sf::Event::MouseButtonPressed &&
                    e.mouseButton.button == sf::Mouse::Left)
                {
                    sf::Vector2f m(e.mouseButton.x, e.mouseButton.y);
                    if (Gfx_OptionsHitTest(m) == 1) {
                        Gfx_SetSkin(previewSkin); // apply chosen skin
                        state = ST_MENU;
                    }
                }
            }

            // ────────────────────────────────────────────
            // 6. PLAY
            else if (state == ST_PLAY) {

                if (e.type == sf::Event::KeyPressed) {

                    if (e.key.code == sf::Keyboard::L) Game_LoadGame();
                    if (e.key.code == sf::Keyboard::T) Game_SaveGame();

                    if (e.key.code == sf::Keyboard::P) Game_TogglePause();                  
                    if (e.key.code == sf::Keyboard::O) Game_ToggleWrap();
                    if (e.key.code == sf::Keyboard::Enter) Game_RestartIfOver();
                    if (e.key.code == sf::Keyboard::Space) { Game_Reset(); Game_SetPaused(false); }

                    Game_OnKeyPressed((int)e.key.code);
                }

                // Mouse handling: if game over show GameOver buttons; otherwise pause menu
                if (e.type == sf::Event::MouseButtonPressed &&
                    e.mouseButton.button == sf::Mouse::Left) {

                    sf::Vector2f m(e.mouseButton.x, e.mouseButton.y);

                    if (Game_Over()) {
                        int action = Gfx_GameOverHitTest(m);
                        if (action == 1) { // Restart
                            Game_Reset();
                            Game_SetPaused(false);
                        }
                        else if (action == 2) { // Back to main menu
                            state = ST_MENU;
                        }
                    }           
                    else {
                        MenuHit hit = Gfx_MenuHitTest(m);

                        if (hit == MH_Resume)       Game_SetPaused(false);
                        else if (hit == MH_Restart) { Game_Reset(); Game_SetPaused(false); }
                        else if (hit == MH_Save)    Game_SaveGame();
                        else if (hit == MH_Load) { if (Game_LoadGame()) Game_SetPaused(false); }
                        else if (hit == MH_Exit)    { Game_Reset(); Game_SetPaused(false); state = ST_MENU; }
                    }
                }
            }

        } // end event loop

        // ======================================================
        // ======================== DRAW =========================
        // ======================================================

        float dt = clock.restart().asSeconds();

        if (state == ST_MENU) {
            Gfx_DrawMainMenu(window, previewSkin);
        }
        else if (state == ST_PROFILE_MENU) {
            Gfx_DrawProfileMenu(window);
        }
        else if (state == ST_NEW_PROFILE) {
            Gfx_DrawNameInput(window, inputName);
        }
        else if (state == ST_SELECT_PROFILE) {
            Gfx_DrawProfileSelect(window, profileList, -1);
        }
        else if (state == ST_OPTIONS) {
            Gfx_DrawOptions(window, previewSkin);
        }
        else if (state == ST_PLAY) {
            Game_Update(dt);
            Gfx_DrawFrame(window);   // Frame này tự window.display()
            continue;                // không cần display lần nữa
        }

        window.display();
    }

    Game_SaveHighScore();
    return 0;
}
