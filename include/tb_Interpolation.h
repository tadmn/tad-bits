
#pragma once

#include "tb_Core.h"
#include "tb_Space.h"

#include <vector>

namespace tb::catmullRom {

/**
 * @brief Calculates the size needed for the output line when calling `spline`
 *
 * @param inLineSize The number of points in the input line (must be >= 4)
 * @param interpolationSteps The number of points to insert between each pair of original points
 * @return The required size for the `outLine` parameter of the `spline` function
 *
 * @note The first and last control points are only used for calculating tangents
 *       and are not included in the output curve
 */
inline int outLineSize(int inLineSize, int interpolationSteps) {
    tb_assert(inLineSize >= 4);
    tb_assert(interpolationSteps > 0);
    inLineSize -= 2; // The control points on either end will not be used
    return inLineSize + (inLineSize - 1) * interpolationSteps;
}

/**
 * @brief Generates a Catmull-Rom spline through the provided control points
 *
 * @param outLine Output vector to store the resulting spline points. This must have the size
 *                calculated by calling outLineSize()
 * @param inLine Input vector containing the control points (must have >= 4 points)
 * @param interpolationSteps Number of points to insert between each pair of original points
 *
 * @details This function creates a smooth curve that passes through all control points
 *          except the first and last ones, which are only used to determine the tangents
 *          at the endpoints. The algorithm inserts interpolationSteps new points between
 *          each pair of original points, using the Catmull-Rom formula to maintain
 *          smoothness across the entire curve.
 *
 * @note The size of outLine must match the value returned by outLineSize() for the same inputs
 */
inline void spline(std::vector<Point>& outLine, const std::vector<Point>& inLine, int interpolationSteps) {
    tb_assert(inLine.size() >= 4);
    tb_assert(outLine.size() == outLineSize(inLine.size(), interpolationSteps));
    tb_assert(interpolationSteps > 0);

    int outIdx = 0;

    // Iterate through points to create interpolated segments
    for (int i = 1; i + 2 < inLine.size(); i++) {
        // Determine control points
        const auto p0 = inLine[i - 1];
        const auto p1 = inLine[i];
        const auto p2 = inLine[i + 1];
        const auto p3 = inLine[i + 2];

        outLine[outIdx] = p1; // Add existing point
        outIdx++;

        // Interpolate between p1 and p2
        for (int j = 1; j <= interpolationSteps && outIdx < outLine.size(); ++j) {
            const auto t = static_cast<double>(j) / (interpolationSteps + 1);

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

            outLine[outIdx] = Point(x, y);
            outIdx++;
        }
    }

    tb_assert(outIdx == outLine.size() - 1);
    outLine[outIdx] = inLine[inLine.size() - 2]; // Add last existing point
}

}