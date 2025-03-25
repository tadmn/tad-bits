
#pragma once

#include <cmath>

namespace tb {

template<typename T>
[[nodiscard]] bool inRange(T value, T min, T max) {
    return value >= min && value <= max;
}

template<typename T>
[[nodiscard]] bool is0to1(T value) {
    return inRange(value, static_cast<T>(0), static_cast<T>(1));
}

template<typename T>
[[nodiscard]] T to0to1(T value, T inMin, T inMax) {
    assert(inRange(value, inMin, inMax));
    value = (value - inMin) / (inMax - inMin);
    assert(is0to1(value));
    return value;
}

template<typename T>
[[nodiscard]] T closestPowerOf2(T input) {
    if (input <= static_cast<T>(0))
        return static_cast<T>(1);

    const auto floorPow2 = std::pow(static_cast<T>(2), std::floor(std::log2(input)));
    const auto ceilingPow2 = std::pow(static_cast<T>(2), std::ceil(std::log2(input)));

    return (input - floorPow2 <= ceilingPow2 - input) ? floorPow2 : ceilingPow2;
}

}