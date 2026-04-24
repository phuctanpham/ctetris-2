// phucpt: app/src/shared/include/SFML/Audio.hpp
// Shim umbrella header — VRSFML loại bỏ file này. Xem comment
// ở src/shared/include/SFML/Graphics.hpp để biết lý do chi tiết.
#pragma once

#if __has_include(<SFML/Audio/AudioContext.hpp>)
    #include <SFML/Audio/AudioContext.hpp>
#endif
#if __has_include(<SFML/Audio/Sound.hpp>)
    #include <SFML/Audio/Sound.hpp>
#endif
#if __has_include(<SFML/Audio/SoundBuffer.hpp>)
    #include <SFML/Audio/SoundBuffer.hpp>
#endif
#if __has_include(<SFML/Audio/SoundChannel.hpp>)
    #include <SFML/Audio/SoundChannel.hpp>
#endif
#if __has_include(<SFML/Audio/SoundSource.hpp>)
    #include <SFML/Audio/SoundSource.hpp>
#endif
#if __has_include(<SFML/Audio/Music.hpp>)
    #include <SFML/Audio/Music.hpp>
#endif