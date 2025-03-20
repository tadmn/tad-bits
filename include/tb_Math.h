
#pragma once

namespace tb {

template <typename T>
[[nodiscard]] T to0to1(T value, T inMin, T inMax) {
  assert(value >= inMin && value <= inMax);
  value = (value - inMin) / (inMax - inMin);
  assert(value >= static_cast<T>(0) && value <= static_cast<T>(1));
  return value;
}

}