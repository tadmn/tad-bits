
#pragma once

namespace tb {

struct Point {
    Point() {}
    Point(float xValue, float yValue) : x(xValue), y(yValue) {}
    float x = 0.f;
    float y = 0.f;
};

inline float distance(Point p0, Point p2) {
    return std::hypot(p2.x - p0.x, p2.y - p0.y);
}

}