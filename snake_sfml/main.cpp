#include <SFML/Graphics.hpp>
#include "config.h"
#include "game.h"
#include "gfx.h"

enum AppState { ST_MENU, ST_OPTIONS, ST_PLAY };

int main() {
    const int WIN_W = (BOARD_W + BORDER * 2) * TILE;
    const int WIN_H = (BOARD_H + BORDER * 2) * TILE;

    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H),
        "Snake SFML  —  Split Files",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setVerticalSyncEnabled(true);

    Game_Init();
    Game_LoadHighScore();
    Gfx_Init(window);
    Gfx_MenuLayout(WIN_W, WIN_H); // bố cục nút

    AppState state = ST_MENU;
    int previewSkin = 0;          // chọn trước skin ở menu/options
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();

            if (state == ST_MENU) {
                // phím nhanh chọn skin
                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code >= sf::Keyboard::Num1 &&
                        e.key.code <= sf::Keyboard::Num6)
                        previewSkin = (int)e.key.code - (int)sf::Keyboard::Num1;
                    if (e.key.code == sf::Keyboard::Enter) {
                        // bấm Enter = Play nhanh
                        Gfx_SetSkin(previewSkin);
                        Game_Reset();
                        state = ST_PLAY;
                    }
                }
                if (e.type == sf::Event::MouseButtonPressed &&
                    e.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f m((float)e.mouseButton.x, (float)e.mouseButton.y);
                    if (Gfx_BtnPlay().contains(m)) {
                        Gfx_SetSkin(previewSkin);
                        Game_Reset();
                        state = ST_PLAY;
                    }
                    else if (Gfx_BtnOptions().contains(m)) {
                        state = ST_OPTIONS;
                    }
                    else if (Gfx_BtnQuit().contains(m)) {
                        window.close();
                    }
                }
            }
            else if (state == ST_OPTIONS) {
                if (e.type == sf::Event::KeyPressed) {
                    // chọn skin
                    if (e.key.code >= sf::Keyboard::Num1 &&
                        e.key.code <= sf::Keyboard::Num6)
                        previewSkin = (int)e.key.code - (int)sf::Keyboard::Num1;

                    // quay về menu
                    if (e.key.code == sf::Keyboard::Enter ||
                        e.key.code == sf::Keyboard::Escape) {
                        state = ST_MENU;
                    }
                }
            }
            else if (state == ST_PLAY) {
                // --- PHẦN BẮT ĐẦU CHƠI: giữ nguyên logic bạn đang có ---
                if (e.type == sf::Event::KeyPressed) {
                    // Chức năng
                    if (e.key.code == sf::Keyboard::P)      Game_TogglePause();
                    if (e.key.code == sf::Keyboard::O)      Game_ToggleWrap();
                    if (e.key.code == sf::Keyboard::Enter)  Game_RestartIfOver();
                    if (e.key.code == sf::Keyboard::Space) { Game_Reset(); Game_SetPaused(false); }

                    // Hướng
                    Game_OnKeyPressed(static_cast<int>(e.key.code));
                }
                if (e.type == sf::Event::MouseMoved) {
                    Gfx_MenuUpdateHover(sf::Vector2f((float)e.mouseMove.x, (float)e.mouseMove.y));
                }
                if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                    auto hit = Gfx_MenuHitTest(sf::Vector2f((float)e.mouseButton.x, (float)e.mouseButton.y));
                    if (hit == MH_Resume)  Game_SetPaused(false);
                    else if (hit == MH_Restart) { Game_Reset(); Game_SetPaused(false); }
                    else if (hit == MH_Exit)    window.close();
                }
            }
        } // pollEvent

        float dt = clock.restart().asSeconds();

        if (state == ST_PLAY) {
            Game_Update(dt);
            Gfx_DrawFrame(window);
        }
        else if (state == ST_MENU) {
            Gfx_DrawMainMenu(window, previewSkin);
            window.display();
        }
        else { // ST_OPTIONS
            Gfx_DrawOptions(window, previewSkin);
            window.display();
        }
    }

    Game_SaveHighScore();
    return 0;
}
