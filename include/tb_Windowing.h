
#pragma once

#include <cmath>
#include <vector>
#include <numbers>

namespace tb {

enum class WindowType {
    Hann,
    BlackmanHarris,
    Hamming
};

template <std::floating_point T>
std::vector<T> window(WindowType windowType, int size) {
    constexpr auto pi = std::numbers::pi;

    std::vector<T> window(size, 0);

    if (windowType == WindowType::Hann) {
        for (size_t i = 0; i < window.size(); ++i)
            window[i] = static_cast<T>(0.5 - std::cos((2.0 * i * pi) / (window.size() - 1)) / 2);
    } else if (windowType == WindowType::BlackmanHarris) {
        constexpr T a0 = 0.35875;
        constexpr T a1 = 0.48829;
        constexpr T a2 = 0.14128;
        constexpr T a3 = 0.01168;

        for (size_t i = 0; i < window.size(); ++i) {
            const auto x = i / static_cast<double>(window.size() - 1);
            window[i] = static_cast<T>(a0 - a1 * std::cos(2.0 * pi * x) + a2 * std::cos(4.0 * pi * x) -
                        a3 * std::cos(6.0 * pi * x));
        }
    } else if (windowType == WindowType::Hamming) {
        for (size_t i = 0; i < window.size(); ++i)
            window[i] = static_cast<T>(0.54 - 0.46 * std::cos((2.0 * i * pi) / (window.size() - 1)));
    } else {
        assert(false);
    }

    return window;
}

}