// phucpt: app/src/shared/include/SFML/Graphics.hpp
// Shim umbrella header — VRSFML loại bỏ file này theo "header hygiene"
// (DESIGN.md: minimize transitive includes). User code dùng <SFML/Graphics.hpp>
// nên shim này ngồi ở src/shared/include/ (được -I trước libs/VRSFML/include).
//
// Dùng __has_include để skip nếu VRSFML không có header đó — tránh lỗi cứng
// khi VRSFML đổi tên file giữa các branch/commit.
#pragma once

#if __has_include(<SFML/Graphics/GraphicsContext.hpp>)
    #include <SFML/Graphics/GraphicsContext.hpp>
#endif
#if __has_include(<SFML/Graphics/RenderWindow.hpp>)
    #include <SFML/Graphics/RenderWindow.hpp>
#endif
#if __has_include(<SFML/Graphics/RenderTarget.hpp>)
    #include <SFML/Graphics/RenderTarget.hpp>
#endif
#if __has_include(<SFML/Graphics/Texture.hpp>)
    #include <SFML/Graphics/Texture.hpp>
#endif
#if __has_include(<SFML/Graphics/Sprite.hpp>)
    #include <SFML/Graphics/Sprite.hpp>
#endif
#if __has_include(<SFML/Graphics/Image.hpp>)
    #include <SFML/Graphics/Image.hpp>
#endif
#if __has_include(<SFML/Graphics/Font.hpp>)
    #include <SFML/Graphics/Font.hpp>
#endif
#if __has_include(<SFML/Graphics/Text.hpp>)
    #include <SFML/Graphics/Text.hpp>
#endif
#if __has_include(<SFML/Graphics/RectangleShape.hpp>)
    #include <SFML/Graphics/RectangleShape.hpp>
#endif
#if __has_include(<SFML/Graphics/CircleShape.hpp>)
    #include <SFML/Graphics/CircleShape.hpp>
#endif
#if __has_include(<SFML/Graphics/Color.hpp>)
    #include <SFML/Graphics/Color.hpp>
#endif
#if __has_include(<SFML/Graphics/View.hpp>)
    #include <SFML/Graphics/View.hpp>
#endif
#if __has_include(<SFML/Graphics/Vertex.hpp>)
    #include <SFML/Graphics/Vertex.hpp>
#endif
#if __has_include(<SFML/Graphics/Rect.hpp>)
    #include <SFML/Graphics/Rect.hpp>
#endif

// Kéo theo Window/System vì VRSFML tách nhỏ hơn SFML standard
#if __has_include(<SFML/Window/Event.hpp>)
    #include <SFML/Window/Event.hpp>
#endif
#if __has_include(<SFML/Window/Keyboard.hpp>)
    #include <SFML/Window/Keyboard.hpp>
#endif
#if __has_include(<SFML/Window/Mouse.hpp>)
    #include <SFML/Window/Mouse.hpp>
#endif
#if __has_include(<SFML/Window/VideoMode.hpp>)
    #include <SFML/Window/VideoMode.hpp>
#endif
#if __has_include(<SFML/Window/WindowBase.hpp>)
    #include <SFML/Window/WindowBase.hpp>
#endif
#if __has_include(<SFML/System/Clock.hpp>)
    #include <SFML/System/Clock.hpp>
#endif
#if __has_include(<SFML/System/Vector2.hpp>)
    #include <SFML/System/Vector2.hpp>
#endif