
#pragma once

namespace tb {

struct Point {
    Point() {}
    Point(float xValue, float yValue) : x(xValue), y(yValue) {}
    float x = 0.f;
    float y = 0.f;
};

}