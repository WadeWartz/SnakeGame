#include <SFML/Graphics.hpp>
using namespace sf;

int main()
{
    RenderWindow window(VideoMode(800, 600), "SFML 2.6.1 Test");
    CircleShape shape(100.f);
    shape.setFillColor(Color::Blue); // Đổi thành màu đỏ

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}
