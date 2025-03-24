
#pragma once

#include <math.h>
#include <vector>

namespace tb {

enum class Window {
  Hann
};

std::vector<float> window(Window windowType, int size) {
  std::vector<float> window(size, 0);
  window.shrink_to_fit();

  if (windowType == Window::Hann) {
    for (size_t i = 0; i < window.size(); ++i)
      window[i] = 0.5 - std::cos((2.0 * i * M_PI) / (window.size() - 1)) / 2;
  } else {
    assert(false);
  }

  return window;
}

}
