
#pragma once

#include "tb_Geometry.h"

#include <vector>
#include <cassert>

namespace tb {

inline void catmullRomSpline(const std::vector<Point>& inLine, std::vector<Point>& outLine) {
    assert(outLine.size() > inLine.size() + 4);

    auto outIdx = 0;

    // Calculate the number of points we'll add in between each existing point
    const auto interpSteps =
        std::max(1, static_cast<int>((outLine.size() / inLine.size()) - 1));

    // Iterate through points to create interpolated segments
    for (int i = 0; i < inLine.size() && outIdx < outLine.size(); i++) {
        // Determine control points
        const auto p0 = inLine[std::max(i - 1, 0)];
        const auto p1 = inLine[i];
        const auto p2 = inLine[std::min(i + 1, static_cast<int>(inLine.size()) - 1)];
        const auto p3 = inLine[std::min(i + 2, static_cast<int>(inLine.size()) - 1)];

        outLine[outIdx++] = p1; // Add existing point

        // Interpolate between p1 and p2
        for (int j = 1; j <= interpSteps && outIdx < outLine.size(); ++j) {
            const auto t = static_cast<double>(j) / (interpSteps + 1);

            // Catmull-Rom spline calculation
            const auto t2 = t * t;
            const auto t3 = t2 * t;

            const auto x = 0.5 * (
                (2.0 * p1.x) +
                (-p0.x + p2.x) * t +
                (2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x) * t2 +
                (-p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x) * t3
            );

            const auto y = 0.5 * (
                (2.0 * p1.y) +
                (-p0.y + p2.y) * t +
                (2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y) * t2 +
                (-p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y) * t3
            );

            outLine[outIdx++] = {x, y};
        }
    }

    // Add last points
    while (outIdx < outLine.size())
        outLine[outIdx++] = inLine.back();
}

}